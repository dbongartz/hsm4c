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
 * | hsm4c                        | UML                   | SCXML                         |
 * |------------------------------|-----------------------|-------------------------------|
 * | HSM4C_TRANSITION_EXTERNAL    | external transition   | type="external"               |
 * | HSM4C_TRANSITION_LOCAL       | local transition      | type="internal"               |
 * | `to == NULL`                 | internal transition   | targetless transition         |
 * | HSM4C_NO_EVENT transition    | completion transition | event-less transition         |
 * | HSM4C_STATE_CHOICE           | choice                | state, event-less transitions |
 * | HSM4C_STATE_HISTORY(_DEEP)   | (deep) history        | history type="shallow"/"deep" |
 * | run_fn (hsm4c_run_fn_t)      | do activity (approx.) | -                             |
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

/** \brief State. See struct hsm4c_state. */
typedef struct hsm4c_state hsm4c_state_t;
/** \brief Constant state configuration. See struct hsm4c_state_config. */
typedef struct hsm4c_state_config hsm4c_state_config_t;
/** \brief Transition table row. See struct hsm4c_transition. */
typedef struct hsm4c_transition hsm4c_transition_t;
/** \brief Event identifier. User events must be > 0, see hsm4c_reserved_event_t. */
typedef int hsm4c_event_id_t;

/** \brief State types */
typedef enum hsm4c_state_type {
  /** \brief Default (simple or composite) state type */
  HSM4C_STATE_NORMAL = 0,

  /**
   * \brief History pseudo state. Must not have entry/exit/run/children.
   *
   * The parent state's last active substate is entered and reset to its initial states.
   * If the parent has no last active substate, the history state's own initial state is
   * entered if set, the parent's initial state otherwise.
   */
  HSM4C_STATE_HISTORY,

  /**
   * \brief Deep history pseudo state. Must not have entry/exit/run/children.
   *
   * The parent state's last active substate and all its last active descendants are
   * entered and NOT reset.
   *
   * If the parent has no last active substate, the history state's own initial state is
   * entered if set, the parent's initial state otherwise.
   */
  HSM4C_STATE_HISTORY_DEEP,

  /**
   * \brief Choice pseudo state.
   *
   * Must not have entry/exit/run/children.
   * Must only have automatic (HSM4C_NO_EVENT) guarded transitions in its own transition table.
   * Guards are evaluated in table order and the first true one is taken.
   * If no guard is true, the statechart stays in the choice state and re-evaluates the
   * guards on every following hsm4c_run(), like an SCXML state with event-less transitions.
   * Use a NULL guard as the last transition for an "else" branch.
   */
  HSM4C_STATE_CHOICE,

  /**
   * \brief Root state of the statechart.
   *
   * Can have entry/exit/run functions and a transition table.
   * Must have an initial state.
   * Must have no parent (NULL).
   * Must not be the target of a transition.
   */
  HSM4C_STATE_ROOT,
} hsm4c_state_type_t;

/** \brief Transition types */
typedef enum hsm4c_transition_type {
  /**
   * \brief External (default) transition.
   *
   * All states from the active leaf up to (excluding) the common ancestor of source and
   * target are exited, then all states down to the target are entered. A self transition
   * exits and re-enters the state.
   */
  HSM4C_TRANSITION_EXTERNAL = 0,

  /**
   * \brief Local transition (SCXML type="internal").
   *
   * - Target is a descendant of the source: the source is not exited, only its active
   *   substates are.
   * - Target is the source ("Same -> Same"): internal transition, same as `to == NULL`.
   * - Any other target: behaves like an external transition.
   */
  HSM4C_TRANSITION_LOCAL,

  /**
   * \brief Marks the end of a transition table. Use HSM4C_TRANSITIONS_END.
   */
  HSM4C_TRANSITION_TABLE_END,
} hsm4c_transition_type_t;

/** \brief Special events. User events must be > 0. */
typedef enum hsm4c_reserved_event {
  /**
   * \brief Use this for event-less / automatic transitions, or as hsm4c_run() event to only poll.
   *
   * Automatic transitions are checked by hsm4c_init() and after every hsm4c_run() event, until
   * none is enabled (SCXML semantics). Their guards are therefore polled until one becomes
   * true. Note: in UML, a completion transition is only checked once, when its source state
   * has been entered.
   */
  HSM4C_NO_EVENT = 0,
} hsm4c_reserved_event_t;

/**
 * \brief Entry function prototype.
 *
 * \param s   State being entered.
 */
typedef void (*hsm4c_entry_fn_t)(hsm4c_state_t const *s);

/**
 * \brief Run function prototype.
 *
 * Called by hsm4c_run() for every state of the active branch, leaf first, whenever no automatic
 * transition is enabled: after the event was processed and again after each transition.
 *
 * \param s   State being run.
 * \param e   Event passed to hsm4c_run().
 *
 * \return    State to trigger an immediate external transition to. NULL, s itself or the
 *            current leaf state to not change state.
 *
 * \attention Transitions triggered by run functions are not visible in the transition
 *            tables. Be careful with this.
 */
typedef hsm4c_state_t *(*hsm4c_run_fn_t)(hsm4c_state_t const *s, hsm4c_event_id_t e);

/**
 * \brief Exit function prototype.
 *
 * \param s   State being exited.
 */
typedef void (*hsm4c_exit_fn_t)(hsm4c_state_t const *s);

/**
 * \brief Transition action prototype.
 *
 * \param root  Root state.
 */
typedef void (*hsm4c_transition_fn_t)(hsm4c_state_t const *root);

/**
 * \brief Guard prototype.
 *
 * \param root  Root state.
 *
 * \return      true if the transition should be taken, false otherwise.
 */
typedef bool (*hsm4c_guard_fn_t)(hsm4c_state_t const *root);

/** \brief Transition. Use designated or positional initializers in transition tables. */
struct hsm4c_transition {
  /**
   * \brief Source state of the transition.
   *
   * In a state's own table it must be that state. In the root's table it can be any state.
   */
  hsm4c_state_t *const from;
  /**
   * \brief Target state of the transition. Must not be the root.
   *
   * NULL for a targetless (internal) transition: only the action is executed, no state is
   * exited or entered.
   */
  hsm4c_state_t *const to;
  /** \brief Event the transition reacts to. Must be positive or HSM4C_NO_EVENT. */
  hsm4c_event_id_t const event;
  /** \brief Transition action. Called after all exits, before all entries. (optional) */
  hsm4c_transition_fn_t const transition_fn;
  /** \brief Guard. Called when source and event match. NULL means always true. (optional) */
  hsm4c_guard_fn_t const guard_fn;
  /** \brief Transition type. Default: HSM4C_TRANSITION_EXTERNAL. */
  hsm4c_transition_type_t type;
};

/** \brief Use this as last element of a transition table. */
#define HSM4C_TRANSITIONS_END                                                                      \
  { .type = HSM4C_TRANSITION_TABLE_END }

/** \brief Constant state configuration. */
struct hsm4c_state_config {
  /** \brief Name of the state. Not used by the library. (optional) */
  char const *name;
  /** \brief Entry function. Called after the exits and the transition action. (optional) */
  hsm4c_entry_fn_t const entry_fn;
  /** \brief Run function. Can trigger a transition. See hsm4c_run_fn_t. (optional) */
  hsm4c_run_fn_t const run_fn;
  /** \brief Exit function. Called after the guard, before the transition action. (optional) */
  hsm4c_exit_fn_t const exit_fn;
  /** \brief Parent state. NULL only for the root state. (mandatory) */
  hsm4c_state_t *const parent;
  /**
   * \brief Initial child state. Entered when this state is a transition target. (optional)
   *
   * For history states: the state entered when there is no history yet.
   */
  hsm4c_state_t *const initial;
  /** \brief State type. See hsm4c_state_type_t. Default: HSM4C_STATE_NORMAL. */
  hsm4c_state_type_t type;
  /**
   * \brief Transition table, terminated by HSM4C_TRANSITIONS_END. (optional)
   *
   * The states of the active branch are searched from the leaf up to the root, so transitions
   * of deeper states take priority. For each state, its own table is searched first, then the
   * rows of the root's table with that state as source. Within a table, the first enabled row
   * is taken.
   */
  hsm4c_transition_t const *transitions;
};

/** \brief State. Holds the runtime data of a state. */
struct hsm4c_state {
  /** \brief Configuration. Set it with hsm4c_map_stateconfig_to_states() or directly. */
  hsm4c_state_config_t const *config;

  /**
   * \brief Private. Active (or last active, for history) child state.
   *
   * On the root this is always the active leaf.
   */
  hsm4c_state_t *_active;
};

/**
 * \brief Initializes a statechart.
 *
 * Enters the root and its initial states down to a leaf, calling their entry functions,
 * then takes automatic transitions until none is enabled. Run functions are not called.
 * History of other states is kept. To reset it, call hsm4c_reset_state() on all states.
 *
 * \param root          Statechart root state.
 *
 * \return              Active leaf state after init.
 */
hsm4c_state_t const *hsm4c_init(hsm4c_state_t *root);

/** \brief Resets the given state, clearing its history. */
void hsm4c_reset_state(hsm4c_state_t *state);

/** \brief Assigns statecfgs[i] to states[i]. For defining states and configs in tables. */
#ifdef __cplusplus
void hsm4c_map_stateconfig_to_states(size_t num_states, hsm4c_state_t states[],
                                     hsm4c_state_config_t const statecfgs[]);
#else
void hsm4c_map_stateconfig_to_states(size_t num_states, hsm4c_state_t states[num_states],
                                     hsm4c_state_config_t const statecfgs[num_states]);
#endif

/**
 * \brief Runs one iteration of the statechart.
 *
 * Takes the enabled transition for the event, if any. Then takes automatic transitions
 * until none is enabled. Whenever no automatic transition is enabled, the run functions of
 * the active branch are called, and can request a further transition.
 *
 * \param root    Statechart root state.
 * \param event   Event to pass to the statechart. Must be > 0, or HSM4C_NO_EVENT to only poll.
 *
 * \return        Active leaf state after the iteration.
 */
hsm4c_state_t const *hsm4c_run(hsm4c_state_t *root, hsm4c_event_id_t event);

/**
 * \brief Gets the root of any state.
 *
 * \param s       State to search from.
 *
 * \return        Root state.
 */
hsm4c_state_t const *hsm4c_get_root(hsm4c_state_t const *s);

#ifdef __cplusplus
}
#endif
