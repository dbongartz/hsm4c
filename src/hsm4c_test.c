#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct State State;
typedef struct Transition Transition;
typedef enum History History;

typedef void (*entry_fn)(State const *sm);
typedef void (*exit_fn)(State const *sm);
typedef void (*transition_fn)(State const *sm);
typedef bool (*guard_fn)(State const *sm);

enum History {
  HSM_NO_HISTORY = 0,
  HSM_HISTORY,
  HSM_HISTORY_DEEP,
};

struct State {
  entry_fn const entry_fn;
  exit_fn const exit_fn;
  State *const parent;
  State *const initial;
  State *current;
  Transition *transitions;
};

struct Transition {
  State *const from;
  State *const to;
  int const event;
  transition_fn const transition_fn;
  guard_fn const guard_fn;
  History history;
};

State *fca(State const *const left, State const *const right) {
  for (State const *_left = left; _left->parent != NULL;
       _left = _left->parent) {
    for (State const *_right = right; _right->parent != NULL;
         _right = _right->parent) {
      if (_left->parent == _right->parent) {
        return _left->parent;
      }
    }
  }
  return NULL;
}

void walk_up_exit(State const *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->parent) {
    start->exit_fn(start);
  }
}

void walk_up_set_current(State *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->parent) {
    start->parent->current = start;
  }
}

State *walk_down_entry(State *start, State const *end_child) {
  State *last_node = start;
  for (State *s = start->current; s != end_child; s = s->current) {
    s->entry_fn(s);
    last_node = s;
  }
  return last_node;
}

void walk_down_init(State *start) {
  for (; start != NULL; start = start->current) {
    start->current = start->initial;
  }
}

State const *find_leaf(State const *start) {
  for (; start->current != NULL; start = start->current) {
  }
  return start;
}

State *sm_init(State *root_state) {
  walk_down_init(root_state);
  root_state->current = walk_down_entry(root_state, NULL);
  return root_state->current;
}

State const *sm_run(State *root, Transition const transitions[], int event) {
  State const *active_state = root->current;
  for (Transition const *t = transitions; t->from != NULL;
       ++t) { // Outer loop can be removed if transitions live in State
    if (t->event == event) {
      for (State const *s = active_state; s != NULL; s = s->parent) {
        if (t->from == s) {
          if (!t->guard_fn || (t->guard_fn && t->guard_fn(active_state))) {
            // Find common ancestor of current leaf and target
            State *ca = fca(active_state, t->to);

            // Exit all states on the current branch until ancestor
            walk_up_exit(active_state, ca);

            // Set target branch current states to reach target
            walk_up_set_current(t->to, ca);

            // Transition
            if (t->transition_fn)
              t->transition_fn(active_state);

            // Entry target branch until target
            walk_down_entry(ca, t->to);

            // Entry target
            t->to->entry_fn(active_state);

            // Handle history
            switch (t->history) {
            case HSM_NO_HISTORY:
              // Init including target
              walk_down_init(t->to);
              break;
            case HSM_HISTORY:
              // Init without target (keeps last active child)
              walk_down_init(t->to->current);
              break;
            case HSM_HISTORY_DEEP:
              // Do not init (keeps last active child of all children)
              break;
            }

            // Walk down until leaf
            root->current = walk_down_entry(t->to, NULL);
            return root->current;
          }
        }
      }
    }
  }

  return active_state;
}

/* ========================== */

static void state_a_entry(State const *sm) { printf("%s\n", __func__); }
static void state_a_exit(State const *sm) { printf("%s\n", __func__); }
static void state_b_entry(State const *sm) { printf("%s\n", __func__); }
static void state_b_exit(State const *sm) { printf("%s\n", __func__); }
static void state_c_entry(State const *sm) { printf("%s\n", __func__); }
static void state_c_exit(State const *sm) { printf("%s\n", __func__); }
static void state_d_entry(State const *sm) { printf("%s\n", __func__); }
static void state_d_exit(State const *sm) { printf("%s\n", __func__); }
static void state_e_entry(State const *sm) { printf("%s\n", __func__); }
static void state_e_exit(State const *sm) { printf("%s\n", __func__); }
static void state_f_entry(State const *sm) { printf("%s\n", __func__); }
static void state_f_exit(State const *sm) { printf("%s\n", __func__); }
static void tran_1(State const *sm) { printf("%s\n", __func__); }
static void tran_2(State const *sm) { printf("%s\n", __func__); }
static bool guard_1(State const *sm) {
  printf("%s\n", __func__);
  return true;
}
static bool guard_2(State const *sm) {
  printf("%s\n", __func__);
  return true;
}

struct MyStateA my_state_a;
struct MyStateB my_state_b;
struct MyStateC my_state_c;
struct MyStateD my_state_d;
struct MyStateE my_state_e;
struct MyStateF my_state_f;

struct MyRoot {
  State const _state;

  char my_char;
} my_sm = {
    ._state = {.entry_fn = NULL,
               .exit_fn = NULL,
               .parent = NULL,
               .initial = (State *)&my_state_a,
               .current = (State *)&my_state_a},
    .my_char = 'a',
};

struct MyStateA {
  State const _state;

  int my_int;
} my_state_a = {
    ._state = {.entry_fn = state_a_entry,
               .exit_fn = state_a_exit,
               .parent = (State *)&my_sm,
               .initial = (State *)&my_state_c,
               .current = (State *)&my_state_c},
    .my_int = 42,
};

struct MyStateB {
  State const _state;

  char my_char;
} my_state_b = {
    ._state = {.entry_fn = state_b_entry,
               .exit_fn = state_b_exit,
               .parent = (State *)&my_sm,
               .initial = NULL,
               .current = NULL},
    .my_char = 'a',
};

struct MyStateC {
  State const _state;

  char my_char;
} my_state_c = {
    ._state = {.entry_fn = state_c_entry,
               .exit_fn = state_c_exit,
               .parent = (State *)&my_state_a,
               .initial = NULL,
               .current = NULL},
    .my_char = 'a',
};

struct MyStateD {
  State const _state;

  char my_char;
} my_state_d = {
    ._state = {.entry_fn = state_d_entry,
               .exit_fn = state_d_exit,
               .parent = (State *)&my_state_a,
               .initial = (State *)&my_state_e,
               .current = (State *)&my_state_e},
    .my_char = 'a',
};

struct MyStateE {
  State const _state;

  char my_char;
} my_state_e = {
    ._state = {.entry_fn = state_e_entry,
               .exit_fn = state_e_exit,
               .parent = (State *)&my_state_d,
               .initial = NULL,
               .current = NULL},
    .my_char = 'a',
};

struct MyStateF {
  State const _state;

  char my_char;
} my_state_f = {
    ._state = {state_f_entry, state_f_exit, (State *)&my_state_d, NULL, NULL},
    .my_char = 'a',
};

static Transition const transitions[] = {
    {(State *)&my_state_a, (State *)&my_state_b, 1, tran_1, guard_1},

    {(State *)&my_state_b, (State *)&my_state_a, 2, tran_2, guard_2},

    {(State *)&my_state_b, (State *)&my_state_a, 3, tran_2, guard_2, HSM_HISTORY},

    {(State *)&my_state_c, (State *)&my_state_d, 4, NULL, NULL, HSM_HISTORY},

    {(State *)&my_state_e, (State *)&my_state_f, 5, NULL, NULL, HSM_HISTORY},

    {
        (State *)&my_state_b,
        (State *)&my_state_a,
        6,
        tran_2,
        guard_2,
        HSM_HISTORY_DEEP,
    },

    {},
};

int main(void) {
  State const *current = NULL;
  current = sm_init((State *)&my_sm);
  current = sm_run((State *)&my_sm, transitions, 1); // B
  current = sm_run((State *)&my_sm, transitions, 2); // A->C
  current = sm_run((State *)&my_sm, transitions, 4); // A->D->E
  current = sm_run((State *)&my_sm, transitions, 1); // B
  current = sm_run((State *)&my_sm, transitions, 3); // A->D->E
  current = sm_run((State *)&my_sm, transitions, 5); // A->D->F
  current = sm_run((State *)&my_sm, transitions, 1); // B
  current = sm_run((State *)&my_sm, transitions,
                   3); // A->D->E (history of A, not of D)
  current = sm_run((State *)&my_sm, transitions, 5); // A->D->F
  current = sm_run((State *)&my_sm, transitions, 1); // B
  current = sm_run((State *)&my_sm, transitions,
                   6); // A->D->F (deep history of A, including D...)
}
