/*
 * Fuzz harness, see fuzz_chart.h.
 *
 * The input bytes are read sequentially: first the chart (states, types, parents, initial
 * states, transition rows and which callbacks are set), then events. Guards, run functions and
 * actions also take their decisions from the input, so the fuzzer explores them too. Once the
 * input is exhausted, every read returns 0.
 *
 * Invariants checked after hsm4c_init() and every hsm4c_run():
 * - Exactly the states on the active branch (leaf up to the root) are entered and not exited.
 * - The runtime array matches the active branch.
 * - The leaf is no history state, and no history state is ever active.
 * And on every entry and exit callback:
 * - A state is entered only after its parent, and only if not entered yet.
 * - A state is exited only if entered and after all its entered children.
 */

#include "fuzz_chart.h"

#include <hsm4c/hsm4c.h>

#include <stdbool.h>
#include <string.h>

#define MAX_STATES 16
#define MAX_ROWS 4
#define NUM_EVENTS 4

struct reader {
  uint8_t const *data;
  size_t size;
  size_t pos;
};

static uint8_t next(struct reader *r) { return r->pos < r->size ? r->data[r->pos++] : 0; }

struct fuzz {
  struct reader input;
  hsm4c_state_t states[MAX_STATES];
  hsm4c_transition_t rows[MAX_STATES][MAX_ROWS + 1];
  hsm4c_chart_t chart;
  bool entered[MAX_STATES];
  bool failed;
};

static struct fuzz *fuzz_of(hsm4c_t const *sm) { return (struct fuzz *)hsm4c_ctx(sm); }

/* -------- Callbacks -------- */

static void entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  struct fuzz *const f = fuzz_of(sm);
  hsm4c_state_id_t const parent = f->states[state].parent;
  if (f->entered[state] || (state != HSM4C_ROOT && !f->entered[parent]) ||
      !hsm4c_is_active(sm, state)) {
    f->failed = true;
  }
  f->entered[state] = true;
  if (next(&f->input) % 4 == 0) {
    (void)hsm4c_raise(sm, 1 + next(&f->input) % NUM_EVENTS, NULL);
  }
}

static void leave(hsm4c_t *sm, hsm4c_state_id_t state) {
  struct fuzz *const f = fuzz_of(sm);
  if (!f->entered[state] || !hsm4c_is_active(sm, state)) {
    f->failed = true;
  }
  for (hsm4c_state_id_t s = 1; s < f->chart.num_states; ++s) {
    if (f->states[s].parent == state && f->entered[s]) {
      f->failed = true; // A child is still entered
    }
  }
  f->entered[state] = false;
}

static bool guard(hsm4c_t const *sm, hsm4c_event_t const *event) {
  (void)event;
  return next(&fuzz_of(sm)->input) % 2 == 0;
}

static void action(hsm4c_t *sm, hsm4c_event_t const *event) {
  (void)event;
  struct fuzz *const f = fuzz_of(sm);
  if (next(&f->input) % 2 == 0) {
    (void)hsm4c_raise(sm, 1 + next(&f->input) % NUM_EVENTS, NULL);
  }
}

static hsm4c_state_id_t run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_t const *event) {
  (void)state;
  (void)event;
  struct fuzz *const f = fuzz_of(sm);
  uint8_t const choice = next(&f->input);
  // Mostly no change, sometimes any state, rarely an invalid one
  if (choice % 4 != 0) {
    return HSM4C_NO_STATE;
  }
  return (hsm4c_state_id_t)(choice / 4 % (f->chart.num_states + 1));
}

/* -------- Chart -------- */

static bool is_pseudo(hsm4c_state_t const *s) { return s->type != HSM4C_STATE_NORMAL; }

/** \brief Returns true if s is a proper descendant of ancestor. */
static bool below(struct fuzz const *f, hsm4c_state_id_t s, hsm4c_state_id_t ancestor) {
  while (s != HSM4C_ROOT) {
    s = f->states[s].parent;
    if (s == ancestor) {
      return true;
    }
  }
  return false;
}

/**
 * \brief Picks a valid initial state, or none. Normal states: a direct child that is no history.
 * Histories: a normal state below their parent, or none if the parent has an initial state.
 * A history without any possible fallback becomes a normal state (it has no children).
 */
static void choose_initial(struct fuzz *f, hsm4c_state_id_t s, hsm4c_state_id_t n) {
  hsm4c_state_t *const state = &f->states[s];
  bool const history =
      state->type == HSM4C_STATE_HISTORY || state->type == HSM4C_STATE_HISTORY_DEEP;

  hsm4c_state_id_t candidates[MAX_STATES];
  unsigned count = 0;
  for (hsm4c_state_id_t c = 1; c < n; ++c) {
    bool const ok = history
                        ? below(f, c, state->parent) && !is_pseudo(&f->states[c])
                        : f->states[c].parent == s && f->states[c].type != HSM4C_STATE_HISTORY &&
                              f->states[c].type != HSM4C_STATE_HISTORY_DEEP;
    if (ok) {
      candidates[count++] = c;
    }
  }

  bool const needs_initial = history && f->states[state->parent].initial == HSM4C_NO_STATE;
  if (needs_initial && count == 0) {
    state->type = HSM4C_STATE_NORMAL;
    return;
  }
  // 0 picks no initial state, unless one is needed
  unsigned const pick = next(&f->input) % (count + 1U);
  if (pick > 0) {
    state->initial = candidates[pick - 1U];
  } else if (needs_initial) {
    state->initial = candidates[0];
  }
}

static void build_chart(struct fuzz *f) {
  struct reader *const r = &f->input;
  hsm4c_state_id_t const n = (hsm4c_state_id_t)(1 + next(r) % MAX_STATES);

  // Tree: each state's parent is an earlier, non-pseudo state. Mostly normal states.
  for (hsm4c_state_id_t s = 1; s < n; ++s) {
    hsm4c_state_id_t parent = (hsm4c_state_id_t)(next(r) % s);
    while (is_pseudo(&f->states[parent])) {
      parent = f->states[parent].parent;
    }
    f->states[s].parent = parent;
    uint8_t const type = next(r) % 8;
    f->states[s].type = type < 5   ? HSM4C_STATE_NORMAL
                        : type < 6 ? HSM4C_STATE_HISTORY
                        : type < 7 ? HSM4C_STATE_HISTORY_DEEP
                                   : HSM4C_STATE_CHOICE;
  }

  // Initial states: normal states first, histories fall back to their parent's
  for (int pass = 0; pass < 2; ++pass) {
    for (hsm4c_state_id_t s = 0; s < n; ++s) {
      bool const history =
          f->states[s].type == HSM4C_STATE_HISTORY || f->states[s].type == HSM4C_STATE_HISTORY_DEEP;
      if ((pass == 0) == (history || f->states[s].type == HSM4C_STATE_CHOICE)) {
        continue;
      }
      choose_initial(f, s, n);
    }
  }

  for (hsm4c_state_id_t s = 0; s < n; ++s) {
    hsm4c_state_t *const state = &f->states[s];

    if (!is_pseudo(state)) {
      state->entry_fn = entry;
      state->exit_fn = leave;
      if (next(r) % 4 == 0) {
        state->run_fn = run;
      }
    }

    if (state->type == HSM4C_STATE_HISTORY || state->type == HSM4C_STATE_HISTORY_DEEP) {
      continue;
    }
    unsigned const rows =
        state->type == HSM4C_STATE_CHOICE ? 1U + next(r) % MAX_ROWS : next(r) % (MAX_ROWS + 1U);
    for (unsigned i = 0; i < rows; ++i) {
      hsm4c_transition_t *const t = &f->rows[s][i];
      t->event = state->type == HSM4C_STATE_CHOICE ? HSM4C_NO_EVENT : next(r) % (NUM_EVENTS + 1);
      t->to = (hsm4c_state_id_t)(next(r) % n);
      t->type = next(r) % 2 ? HSM4C_TRANSITION_LOCAL : HSM4C_TRANSITION_EXTERNAL;
      uint8_t const fns = next(r);
      t->guard_fn = fns & 1U ? guard : NULL;
      t->action_fn = fns & 2U ? action : NULL;
    }
    f->rows[s][rows].type = HSM4C_TRANSITION_TABLE_END;
    state->transitions = f->rows[s];
  }

  f->chart.states = f->states;
  f->chart.num_states = n;
}

/* -------- Invariants -------- */

static bool consistent(struct fuzz const *f, hsm4c_t const *sm) {
  hsm4c_state_id_t const leaf = hsm4c_current(sm);
  if (leaf >= f->chart.num_states || f->states[leaf].type == HSM4C_STATE_HISTORY ||
      f->states[leaf].type == HSM4C_STATE_HISTORY_DEEP) {
    return false;
  }

  // The active branch: entered, and the runtime array points along it
  bool on_branch[MAX_STATES] = {false};
  hsm4c_state_id_t s = leaf;
  on_branch[s] = true;
  while (s != HSM4C_ROOT) {
    hsm4c_state_id_t const parent = f->states[s].parent;
    if (sm->active[parent] != s) {
      return false;
    }
    s = parent;
    on_branch[s] = true;
  }

  for (hsm4c_state_id_t i = 0; i < f->chart.num_states; ++i) {
    bool const history =
        f->states[i].type == HSM4C_STATE_HISTORY || f->states[i].type == HSM4C_STATE_HISTORY_DEEP;
    // Entered is tracked for normal states, choices have no callbacks
    bool const tracked = !is_pseudo(&f->states[i]);
    if ((tracked && f->entered[i] != on_branch[i]) || (history && hsm4c_is_active(sm, i))) {
      return false;
    }
  }
  return true;
}

/* -------- Entry point -------- */

enum fuzz_result fuzz_run(uint8_t const *data, size_t size) {
  struct fuzz f;
  memset(&f, 0, sizeof(f));
  f.input.data = data;
  f.input.size = size;

  build_chart(&f);
  if (hsm4c_validate(&f.chart, NULL) != HSM4C_OK) {
    return FUZZ_SKIPPED;
  }

  hsm4c_t sm;
  hsm4c_state_id_t active[MAX_STATES];
  (void)hsm4c_init(&sm, &f.chart, active, &f);
  if (f.failed || !consistent(&f, &sm)) {
    return FUZZ_FAILED;
  }

  // Events until the input is exhausted, sometimes raised from outside first
  while (f.input.pos < f.input.size) {
    uint8_t const op = next(&f.input);
    if (op % 8 == 0) {
      (void)hsm4c_raise(&sm, 1 + next(&f.input) % NUM_EVENTS, NULL);
    }
    (void)hsm4c_run(&sm, (hsm4c_event_id_t)(op % (NUM_EVENTS + 1)), NULL);
    if (f.failed || !consistent(&f, &sm)) {
      return FUZZ_FAILED;
    }
  }
  return FUZZ_OK;
}
