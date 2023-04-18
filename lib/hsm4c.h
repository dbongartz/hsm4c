#ifndef __HSM4C__H__
#define __HSM4C__H__

// #include <stddef.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct State State;
typedef struct Transition Transition;
typedef int EventType;

typedef enum StateType {
  SC_TYPE_NORMAL = 0,
  SC_TYPE_HISTORY,
  SC_TYPE_HISTORY_DEEP,
  SC_TYPE_CHOICE,
  SC_TYPE_ROOT,
} StateType;

typedef enum TransitionType {
  SC_TTYPE_EXTERNAL = 0,
  SC_TTYPE_INTERNAL,
} TransitionType;

typedef void (*entry_fn)(State const *sm);
typedef State *(*run_fn)(State const *sm, EventType e);
typedef void (*exit_fn)(State const *sm);
typedef void (*transition_fn)(State const *sm);
typedef bool (*guard_fn)(State const *sm);

#define SC_TRANSITIONS_END ((Transition const){})

#define SC_NO_EVENT ((EventType)0)

struct Transition {
  State *const from;
  State *const to;
  int const event;
  transition_fn const transition_fn;
  guard_fn const guard_fn;
  TransitionType type;
};

struct State {
  char const *name;
  entry_fn const entry_fn;
  run_fn const run_fn;
  exit_fn const exit_fn;
  State *const parent;
  State *const initial;
  StateType type;

  /* Private */
  /** \brief Active child state. On root node this is always a leaf */
  State *_active;
  Transition const *_transitions;
};

State *sc_init(State *root_state, Transition const transitions[]);
void sc_reset_state(State *state);

State const *sc_run(State *root, EventType event);

#endif
