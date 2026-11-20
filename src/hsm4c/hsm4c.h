/**
 * \brief Statechart implementation in C
 * \file
 *
 * Tries to make statecharts accessible in C.
 *
 * The structure (states, pseudo states, transition kinds) follows UML 2. The execution
 * semantics follow SCXML (W3C): event transitions are taken first, then automatic
 * (event-less) transitions until none is enabled.
 *
 * Terminology mapping:
 *
 * | hsm4c                         | UML                   | SCXML                           |
 * |-------------------------------|-----------------------|---------------------------------|
 * | SC_TTYPE_EXTERNAL             | external transition   | type="external"                 |
 * | SC_TTYPE_LOCAL                | local transition      | type="internal"                 |
 * | `to == NULL`                  | internal transition   | targetless transition           |
 * | SC_NO_EVENT transition        | completion transition | event-less transition           |
 * | SC_TYPE_CHOICE                | choice                | state with event-less transitions |
 * | SC_TYPE_HISTORY(_DEEP)        | (deep) history        | history type="shallow"/"deep"   |
 * | run_fn (sc_run_fn)            | do activity (approx.) | -                               |
 *
 * Supports:
 *
 * - Fully hierarchical statecharts.
 * - Entry/exit/run state actions.
 * - Run functions can trigger an immediate transition.
 * - Transitions with guard and action.
 * - External, local and internal (targetless) transitions.
 * - Automatic (event-less) transitions.
 * - Initial child states.
 * - History and deep history pseudo states with an optional initial state.
 * - Choice pseudo states.
 * - Relatively easy table-based syntax (see examples/hsm4c_demo.c).
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

/** \brief State. See struct State. */
typedef struct State State;
/** \brief Constant state configuration. See struct StateConfig. */
typedef struct StateConfig StateConfig;
/** \brief Transition table row. See struct Transition. */
typedef struct Transition Transition;
/** \brief Event identifier. User events must be > 0, see ScEvents. */
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
   * If no guard is true, the statechart stays in the choice state and re-evaluates the
   * guards on every following sc_run(), like an SCXML state with event-less transitions.
   * Use a NULL guard as the last transition for an "else" branch.
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
   * \brief Local transition (SCXML type="internal").
   *
   * - Target is a descendant of the source: the source is not exited, only its active
   *   substates are.
   * - Target is the source ("Same -> Same"): internal transition, same as `to == NULL`.
   * - Any other target: behaves like an external transition.
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
   * Automatic transitions are checked by sc_init() and after every sc_run() event, until
   * none is enabled (SCXML semantics). Their guards are therefore polled until one becomes
   * true. Note: in UML, a completion transition is only checked once, when its source state
   * has been entered.
   */
  SC_NO_EVENT = 0,
} ScEvents;

/**
 * \brief Entry function prototype.
 *
 * \param s   State being entered.
 */
typedef void (*sc_entry_fn)(State const *s);

/**
 * \brief Run function prototype.
 *
 * Called by sc_run() for every state of the active branch, leaf first, whenever no automatic
 * transition is enabled: after the event was processed and again after each transition.
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
typedef State *(*sc_run_fn)(State const *s, EventType e);

/**
 * \brief Exit function prototype.
 *
 * \param s   State being exited.
 */
typedef void (*sc_exit_fn)(State const *s);

/**
 * \brief Transition action prototype.
 *
 * \param root  Root state.
 */
typedef void (*sc_transition_fn)(State const *root);

/**
 * \brief Guard prototype.
 *
 * \param root  Root state.
 *
 * \return      true if the transition should be taken, false otherwise.
 */
typedef bool (*sc_guard_fn)(State const *root);

/** \brief Transition. Use designated or positional initializers in transition tables. */
struct Transition {
  /**
   * \brief Source state of the transition.
   *
   * In a state's own table it must be that state. In the root's table it can be any state.
   */
  State *const from;
  /**
   * \brief Target state of the transition. Must not be the root.
   *
   * NULL for a targetless (internal) transition: only the action is executed, no state is
   * exited or entered.
   */
  State *const to;
  /** \brief Event the transition reacts to. Must be positive or SC_NO_EVENT. */
  EventType const event;
  /** \brief Transition action. Called after all exits, before all entries. (optional) */
  sc_transition_fn const transition_fn;
  /** \brief Guard. Called when source and event match. NULL means always true. (optional) */
  sc_guard_fn const guard_fn;
  /** \brief Transition type. Default: SC_TTYPE_EXTERNAL. */
  TransitionType type;
};

/** \brief Use this as last element of a transition table. */
#define SC_TRANSITIONS_END                                                                         \
  { .type = SC_TTYPE_TABLE_END }

/** \brief Constant state configuration. */
struct StateConfig {
  /** \brief Name of the state. Not used by the library. (optional) */
  char const *name;
  /** \brief Entry function. Called after the exits and the transition action. (optional) */
  sc_entry_fn const entry_fn;
  /** \brief Run function. Can trigger a transition. See sc_run_fn. (optional) */
  sc_run_fn const run_fn;
  /** \brief Exit function. Called after the guard, before the transition action. (optional) */
  sc_exit_fn const exit_fn;
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
   * The states of the active branch are searched from the leaf up to the root, so transitions
   * of deeper states take priority. For each state, its own table is searched first, then the
   * rows of the root's table with that state as source. Within a table, the first enabled row
   * is taken.
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
 * Enters the root and its initial states down to a leaf, calling their entry functions,
 * then takes automatic transitions until none is enabled. Run functions are not called.
 * History of other states is kept. To reset it, call sc_reset_state() on all states.
 *
 * \param root          Statechart root state.
 *
 * \return              Active leaf state after init.
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
 * Takes the enabled transition for the event, if any. Then takes automatic transitions
 * until none is enabled. Whenever no automatic transition is enabled, the run functions of
 * the active branch are called, and can request a further transition.
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
