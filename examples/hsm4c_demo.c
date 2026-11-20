/**
 * hsm4c demo
 *
 * ROOT
 * +- A (initial C)
 * |  +- A_H     shallow history
 * |  +- A_HD    deep history (initial C)
 * |  +- BRANCH  run function transitions to D
 * |  +- C
 * |  +- D (initial E)
 * |     +- D_H  shallow history (initial E)
 * |     +- E
 * |     +- F
 * +- B
 * +- G (initial GA)
 *    +- G_HD    deep history (initial GB)
 *    +- GA
 *    +- GB
 */

#include <hsm4c/hsm4c.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum my_states { ROOT, A, A_H, A_HD, BRANCH, B, C, D, D_H, E, F, G, G_HD, GA, GB, NUM_STATES };

static char const *state_name(hsm4c_state_id_t state);

/* -------- State functions -------- */

static void entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)sm;
  printf("  entry  %s\n", state_name(state));
}

static void exit_(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)sm;
  printf("  exit   %s\n", state_name(state));
}

static hsm4c_state_id_t run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_t const *e) {
  (void)sm;
  printf("  run    %s (event %d)\n", state_name(state), e->id);
  return HSM4C_NO_STATE;
}

static hsm4c_state_id_t branch_run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_t const *e) {
  (void)sm;
  printf("  run    %s (event %d) -> D\n", state_name(state), e->id);
  return D;
}

/* -------- Transition functions -------- */

static void action(hsm4c_t *sm, hsm4c_event_t const *e) {
  printf("  action (in %s, event %d)\n", state_name(hsm4c_current(sm)), e->id);
}

static bool guard(hsm4c_t const *sm, hsm4c_event_t const *e) {
  printf("  guard  (in %s, event %d)\n", state_name(hsm4c_current(sm)), e->id);
  return true;
}

/** \brief Application data, reached from callbacks via hsm4c_ctx(). */
struct app {
  int condition_calls;
};

/** \brief False on the first call, true afterwards. */
static bool condition(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  struct app *const app = hsm4c_ctx(sm);
  printf("  condition (in %s): call %d\n", state_name(hsm4c_current(sm)), app->condition_calls);
  return app->condition_calls++ >= 1;
}

/* -------- Statechart -------- */

static hsm4c_transition_t const a_transitions[] = {
    {.event = 1, .to = B, .guard_fn = guard, .action_fn = action},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const b_transitions[] = {
    {.event = 2, .to = A, .guard_fn = guard, .action_fn = action},
    {.event = 3, .to = A_H, .guard_fn = guard, .action_fn = action},
    {.event = 6, .to = A_HD, .guard_fn = guard, .action_fn = action},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const c_transitions[] = {
    {.event = 4, .to = D_H},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const e_transitions[] = {
    {.event = 5, .to = F},
    {.event = 7, .to = G_HD},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const f_transitions[] = {
    {.event = 7, .to = BRANCH},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const ga_transitions[] = {
    {.event = HSM4C_NO_EVENT, .to = A, .guard_fn = condition},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_transition_t const gb_transitions[] = {
    {.event = HSM4C_NO_EVENT, .to = GA, .guard_fn = condition},
    HSM4C_TRANSITIONS_END,
};

#define STATE(n, ...)                                                                              \
  { .name = n, .entry_fn = entry, .exit_fn = exit_, __VA_ARGS__ }

/* .parent defaults to ROOT (0) */
static hsm4c_state_t const my_states[NUM_STATES] = {
    [ROOT] = {.name = "ROOT", .initial = A},
    [A] = STATE("A", .initial = C, .transitions = a_transitions),
    [A_H] = {.name = "A_H", .parent = A, .type = HSM4C_STATE_HISTORY},
    [A_HD] = {.name = "A_HD", .parent = A, .initial = C, .type = HSM4C_STATE_HISTORY_DEEP},
    [BRANCH] = {.name = "BRANCH", .parent = A, .run_fn = branch_run},
    [B] = STATE("B", .run_fn = run, .transitions = b_transitions),
    [C] = STATE("C", .parent = A, .transitions = c_transitions),
    [D] = STATE("D", .parent = A, .initial = E),
    [D_H] = {.name = "D_H", .parent = D, .initial = E, .type = HSM4C_STATE_HISTORY},
    [E] = STATE("E", .parent = D, .transitions = e_transitions),
    [F] = STATE("F", .parent = D, .run_fn = run, .transitions = f_transitions),
    [G] = STATE("G", .initial = GA),
    [G_HD] = {.name = "G_HD", .parent = G, .initial = GB, .type = HSM4C_STATE_HISTORY_DEEP},
    [GA] = STATE("GA", .parent = G, .transitions = ga_transitions),
    [GB] = STATE("GB", .parent = G, .transitions = gb_transitions),
};

static hsm4c_chart_t const my_chart = HSM4C_CHART(my_states);

static char const *state_name(hsm4c_state_id_t state) { return my_states[state].name; }

/* -------- Main -------- */

struct step {
  hsm4c_event_id_t event;
  char const *expected;
};

int main(void) {
  static struct step const steps[] = {
      {1, "B"},
      {HSM4C_NO_EVENT, "B (no change)"},
      {2, "C (A initial)"},
      {4, "E (D_H without history -> its initial E)"},
      {1, "B"},
      {3, "E (A_H: history of A is D, D is reset)"},
      {5, "F"},
      {1, "B"},
      {3, "E (A_H: history of A, not of D)"},
      {5, "F"},
      {1, "B"},
      {6, "F (A_HD: deep history of A, including D)"},
      {7, "E (F -> BRANCH, run of BRANCH -> D)"},
      {7, "GB (G_HD without history -> its initial GB, condition false)"},
      {7, "C (condition true: GB -> GA -> A)"},
      {8, "C (no change, no transition for event 8)"},
      {HSM4C_NO_EVENT, "C (no change)"},
  };

  printf("hsm4c demo\n");
  printf("sizeof(hsm4c_state_t): %zu, sizeof(hsm4c_transition_t): %zu\n\n", sizeof(hsm4c_state_t),
         sizeof(hsm4c_transition_t));

  struct app app = {0};
  hsm4c_t sm;
  hsm4c_state_id_t active[NUM_STATES];

  printf("hsm4c_init()\n");
  if (hsm4c_init(&sm, &my_chart, active, &app) != HSM4C_OK) {
    return 1;
  }
  printf("-> %s\n\n", state_name(hsm4c_current(&sm)));

  for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    printf("hsm4c_run(%d), expected: %s\n", steps[i].event, steps[i].expected);
    if (hsm4c_run(&sm, steps[i].event, NULL) != HSM4C_OK) {
      return 1;
    }
    printf("-> %s\n\n", state_name(hsm4c_current(&sm)));
  }

  return 0;
}
