/**
 * \brief Statechart implementation in C
 * \file
 *
 * Tries to make UML 2 statecharts accessible in C.
 *
 * Supports:
 *
 * - Fully hierarchical statecharts.
 * - Entry/exit/run state actions.
 * - Run functions can trigger an immediate transition.
 * - Transitions with guard and action.
 * - External, local and internal transitions.
 * - Automatic (event-less) transitions.
 * - Initial child states.
 * - History and deep history pseudo states with an optional initial state.
 * - Choice pseudo states.
 * - Relatively easy table-based syntax (see test/test_chart.c).
 *
 * Does not support:
 * - Parallel (orthogonal) states.
 *
 * (C) 2023 David Bongartz
 * MIT License
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct State State;
typedef struct StateConfig StateConfig;
typedef struct Transition Transition;
typedef int EventType;

/** \brief State Types */
typedef enum StateType {
  /** \brief Default (simple or composite) state type */
  SC_TYPE_NORMAL = 0,

  /**
   * \brief History pseudo state. Must not have entry/exit/run/children.
   *
   * The parent state's last active substate is entered and reset to its initial states.
   * If the parent has no last active substate, the history state's own initial state is
   * entered if set, the parent's initial state otherwise.
   */
  SC_TYPE_HISTORY,

  /**
   * \brief Deep history pseudo state. Must not have entry/exit/run/children.
   *
   * The parent state's last active substate and all its last active descendants are
   * entered and NOT reset.
   *
   * If the parent has no last active substate, the history state's own initial state is
   * entered if set, the parent's initial state otherwise.
   */
  SC_TYPE_HISTORY_DEEP,

  /**
   * \brief Choice pseudo state.
   *
   * Must not have entry/exit/run/children.
   * Must only have automatic (SC_NO_EVENT) guarded transitions in its own transition table.
   * Guards are evaluated in table order and the first true one is taken.
   * At least one guard must be true (use a NULL guard as the last "else" transition).
   * If none is true, the behaviour is undefined.
   */
  SC_TYPE_CHOICE,

  /**
   * \brief Root state of the statechart.
   *
   * Can have entry/exit/run functions and a transition table.
   * Must have an initial state.
   * Must have no parent (NULL).
   * Must not be the target of a transition.
   */
  SC_TYPE_ROOT,
} StateType;

/** \brief Transition Types */
typedef enum TransitionType {
  /**
   * \brief External (default) transition.
   *
   * All states from the active leaf up to (excluding) the common ancestor of source and
   * target are exited, then all states down to the target are entered. A self transition
   * exits and re-enters the state.
   */
  SC_TTYPE_EXTERNAL = 0,

  /**
   * \brief Local transition. The containing state is not exited and re-entered.
   *
   * Must be "Parent -> Child", "Child -> Parent" or "Same -> Same".
   * - "Parent -> Child": the parent is not exited, its active substates are.
   * - "Child -> Parent": the parent is not exited/entered, only re-initialized.
   * - "Same -> Same": internal transition. Only the action is executed, no state is exited
   *   or entered and the active states do not change.
   */
  SC_TTYPE_LOCAL,

  /**
   * \brief Marks the end of a transition table. Use SC_TRANSITIONS_END.
   */
  SC_TTYPE_TABLE_END,
} TransitionType;

/** \brief Special events. User events must be > 0. */
typedef enum ScEvents {
  /**
   * \brief Use this for event-less / automatic transitions, or as sc_run() event to only poll.
   *
   * Automatic transitions are checked after every transition and on every sc_run() call,
   * whatever the event. Their guards are therefore polled until one becomes true.
   * Note: this follows SCXML. In UML, a completion transition is only checked once, when its
   * source state has been entered.
   */
  SC_NO_EVENT = 0,
} ScEvents;

/**
 * \brief Entry function prototype.
 *
 * \param s   State being entered.
 */
typedef void (*entry_fn)(State const *s);

/**
 * \brief Run function prototype.
 *
 * Called for every state of the active branch, leaf first, when sc_run() takes no table
 * transition, and again after each transition.
 *
 * \param s   State being run.
 * \param e   Event passed to sc_run().
 *
 * \return    State to trigger an immediate external transition to. NULL, s itself or the
 *            current leaf state to not change state.
 *
 * \attention Transitions triggered by run functions are not visible in the transition
 *            tables. Be careful with this.
 */
typedef State *(*run_fn)(State const *s, EventType e);

/**
 * \brief Exit function prototype.
 *
 * \param s   State being exited.
 */
typedef void (*exit_fn)(State const *s);

/**
 * \brief Transition action prototype.
 *
 * \param root  Root state.
 */
typedef void (*transition_fn)(State const *root);

/**
 * \brief Guard prototype.
 *
 * \param root  Root state.
 *
 * \return      true if the transition should be taken, false otherwise.
 */
typedef bool (*guard_fn)(State const *root);

/** \brief Transition. Use designated or positional initializers in transition tables. */
struct Transition {
  /**
   * \brief Source state of the transition.
   *
   * In a state's own table it must be that state. In the root's table it can be any state.
   */
  State *const from;
  /** \brief Target state of the transition. Must be a valid state other than the root. */
  State *const to;
  /** \brief Event the transition reacts to. Must be positive or SC_NO_EVENT. */
  EventType const event;
  /** \brief Transition action. Called after all exits, before all entries. (optional) */
  transition_fn const transition_fn;
  /** \brief Guard. Called when source and event match. NULL means always true. (optional) */
  guard_fn const guard_fn;
  /** \brief Transition type. Default: SC_TTYPE_EXTERNAL. */
  TransitionType type;
};

/** \brief Use this to indicate the end of the transition table. */
#define SC_TRANSITIONS_END ((Transition const){.type = SC_TTYPE_TABLE_END})

/** \brief Constant state configuration. */
struct StateConfig {
  /** \brief Name of the state. Not used by the library. (optional) */
  char const *name;
  /** \brief Entry function. Called after the exits and the transition action. (optional) */
  entry_fn const entry_fn;
  /** \brief Run function. Can trigger a transition. See run_fn. (optional) */
  run_fn const run_fn;
  /** \brief Exit function. Called after the guard, before the transition action. (optional) */
  exit_fn const exit_fn;
  /** \brief Parent state. NULL only for the root state. (mandatory) */
  State *const parent;
  /**
   * \brief Initial child state. Entered when this state is a transition target. (optional)
   *
   * For history states: the state entered when there is no history yet.
   */
  State *const initial;
  /** \brief State type. See StateType. Default: SC_TYPE_NORMAL. */
  StateType type;
  /**
   * \brief Transition table, terminated by SC_TRANSITIONS_END. (optional)
   *
   * Tables are searched from the active leaf up to the root, so transitions of inner states
   * take priority. In the root's table, the first matching row whose source is in the active
   * branch is taken.
   */
  Transition const *transitions;
};

/** \brief State. Holds the runtime data of a state. */
struct State {
  /** \brief Configuration. Set it with sc_map_stateconfig_to_states() or directly. */
  StateConfig const *config;

  /**
   * \brief Private. Active (or last active, for history) child state.
   *
   * On the root this is always the active leaf.
   */
  State *_active;
};

/**
 * \brief Initializes a statechart.
 *
 * Enters the root and its initial states down to a leaf, calling their entry functions.
 * History of other states is kept. To reset it, call sc_reset_state() on all states.
 *
 * \param root          Statechart root state.
 *
 * \return              Leaf state after init.
 */
State const *sc_init(State *root);

/** \brief Resets the given state, clearing its history. */
void sc_reset_state(State *state);

/** \brief Assigns statecfgs[i] to states[i]. For defining states and configs in tables. */
#ifdef __cplusplus
void sc_map_stateconfig_to_states(size_t num_states, State states[], StateConfig const statecfgs[]);
#else
void sc_map_stateconfig_to_states(size_t num_states, State states[num_states],
                                  StateConfig const statecfgs[num_states]);
#endif

/**
 * \brief Runs one iteration of the statechart.
 *
 * Takes the first matching transition, followed by all enabled automatic transitions.
 * Run functions are called when no transition is taken and after each transition.
 *
 * \param root    Statechart root state.
 * \param event   Event to pass to the statechart. Must be > 0, or SC_NO_EVENT to only poll.
 *
 * \return        Active leaf state after the iteration.
 */
State const *sc_run(State *root, EventType event);

/**
 * \brief Gets the root of any state.
 *
 * \param s       State to search from.
 *
 * \return        Root state.
 */
State const *sc_get_root(State const *s);

#ifdef __cplusplus
}
#endif
