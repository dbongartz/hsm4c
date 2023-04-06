#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct State State;
typedef struct Transition Transition;

typedef void (*entry_fn)(State *sm);
typedef void (*exit_fn)(State *sm);
typedef void (*transition_fn)(State *sm);
typedef bool (*guard_fn)(State *sm);

struct State {
  entry_fn const entry_fn;
  exit_fn const exit_fn;
  State *const parent;
  State *const initial;
  State *current;
};

struct Transition {
  State *const from;
  State *const to;
  int const event;
  bool history;
  transition_fn const transition_fn;
  guard_fn const guard_fn;
};

// struct StateMachine {
//   State *const initial;
//   State *current;

//   Transition const *const transitions;
// };

State *fca(State const *left, State const *right) {
  for (; left->parent != NULL; left = left->parent) {
    for (; right->parent != NULL; right = right->parent) {
      if (left->parent == right->parent) {
        return left->parent;
      }
    }
  }
  return NULL;
}

void walk_up_exit(State *sm, State const *node, State const *ancestor) {
  for (; node != ancestor; node = node->parent) {
    node->exit_fn(sm);
  }
}

void walk_up_set_current(State *sm, State *node, State const *ancestor) {
  for (; node != ancestor; node = node->parent) {
    node->parent->current = node;
  }
}

void walk_down_entry(State *sm, State const *node, State const *child) {
  for (; node != child; node = node->current) {
    node->current->entry_fn(sm);
  }
}

State *sm_run(State *current, Transition transitions[], int event) {
  for (Transition const *t = transitions; t->from != NULL; ++t) {
    if (t->from == current) {
      if (t->event == event) {
        if (t->guard_fn(current)) {
          State *ca = fca(t->from, t->to);

          walk_up_exit(current, t->from, ca);
          walk_up_set_current(current, t->to, ca);

          if (t->transition_fn)
            t->transition_fn(current);

          walk_down_entry(current, ca, t->to);

          if (t->history) {
            t->to->current = t->to->initial;
          }

          return t->to->current ? t->to->current : t->to;
        }
      }
    }
  }

  return current;
}

/* ========================== */

void state_a_entry(State *sm) { printf("%s\n", __func__); }
void state_a_exit(State *sm) { printf("%s\n", __func__); }
void state_b_entry(State *sm) { printf("%s\n", __func__); }
void state_b_exit(State *sm) { printf("%s\n", __func__); }
void tran_1(State *sm) { printf("%s\n", __func__); }
void tran_2(State *sm) { printf("%s\n", __func__); }
bool guard_1(State *sm) {
  printf("%s\n", __func__);
  return true;
}
bool guard_2(State *sm) {
  printf("%s\n", __func__);
  return true;
}

struct MyStateA my_state_a;
struct MyStateB my_state_b;

struct MyRoot {
  State const _state;

  char my_char;
} my_sm = {
    ._state = {.entry_fn = NULL,
               .exit_fn = NULL,
               .current = (State *)&my_state_a,
               .initial = (State *)&my_state_a,
               .parent = NULL},
    .my_char = 'a',
};

struct MyStateA {
  State const _state;

  int my_int;
} my_state_a = {
    ._state = {.entry_fn = state_a_entry,
               .exit_fn = state_a_exit,
               .current = NULL,
               .initial = NULL,
               .parent = (State *)&my_sm},
    .my_int = 42,
};

struct MyStateB {
  State const _state;

  char my_char;
} my_state_b = {
    ._state = {.entry_fn = state_b_entry,
               .exit_fn = state_b_exit,
               .current = NULL,
               .initial = NULL,
               .parent = (State *)&my_sm},
    .my_char = 'a',
};

Transition transitions[] = {{.from = (State *)&my_state_a,
                             .to = (State *)&my_state_b,
                             .event = 1,
                             .guard_fn = guard_1,
                             .transition_fn = tran_1},

                            {.from = (State *)&my_state_b,
                             .to = (State *)&my_state_a,
                             .event = 2,
                             .guard_fn = guard_2,
                             .transition_fn = tran_2},

                            {}};

int main(void) {
  State *current = my_sm._state.initial;
  current = sm_run(current, transitions, 1);
  current = sm_run(current, transitions, 2);
  current = sm_run(current, transitions, 2);
  current = sm_run(current, transitions, 1);
  current = sm_run(current, transitions, 2);
}
