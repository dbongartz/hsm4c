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
 * A statechart is a constant array of states (hsm4c_state_t), indexed by state ID. State 0 is
 * always the root. Each state can have its own transition table. A machine (hsm4c_t) runs a
 * chart and keeps its runtime data in an array of state IDs provided by the user, so one chart
 * can drive several machines and can be placed in read-only memory.
 *
 * Terminology mapping:
 *
 * | hsm4c                        | UML                   | SCXML                         |
 * |------------------------------|-----------------------|-------------------------------|
 * | HSM4C_TRANSITION_EXTERNAL    | external transition   | type="external"               |
 * | HSM4C_TRANSITION_LOCAL       | local transition      | type="internal"               |
 * | `to == HSM4C_NO_STATE`       | internal transition   | targetless transition         |
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HSM4C_STATE_ID_TYPE
/** \brief Unsigned integer type of state IDs. Define it before including to change it. */
#define HSM4C_STATE_ID_TYPE uint16_t
#endif

/** \brief State ID: index into the chart's state array. */
typedef HSM4C_STATE_ID_TYPE hsm4c_state_id_t;

/** \brief Event identifier. User events must be > 0, see hsm4c_reserved_event_t. */
typedef int hsm4c_event_id_t;

/** \brief State configuration. See struct hsm4c_state. */
typedef struct hsm4c_state hsm4c_state_t;
/** \brief Transition table row. See struct hsm4c_transition. */
typedef struct hsm4c_transition hsm4c_transition_t;
/** \brief Statechart. See struct hsm4c_chart. */
typedef struct hsm4c_chart hsm4c_chart_t;
/** \brief Machine running a statechart. See struct hsm4c. */
typedef struct hsm4c hsm4c_t;

/** \brief The root state. It is always state 0. */
#define HSM4C_ROOT ((hsm4c_state_id_t)0)

/**
 * \brief "No state" for initial states, transition targets and run function results.
 *
 * Same value as HSM4C_ROOT. The root can never be an initial state, a transition target or a
 * child, so 0 is free to mean "none" there. Zero-initialized fields therefore mean "none".
 */
#define HSM4C_NO_STATE ((hsm4c_state_id_t)0)

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
   * - Target is the source ("Same -> Same"): internal transition, same as
   *   `to == HSM4C_NO_STATE`.
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
   * \brief Use this for event-less / automatic transitions, or as hsm4c_run() event to only
   * poll.
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
 * \param sm      Machine.
 * \param state   State being entered.
 */
typedef void (*hsm4c_entry_fn_t)(hsm4c_t *sm, hsm4c_state_id_t state);

/**
 * \brief Run function prototype.
 *
 * Called by hsm4c_run() for every state of the active branch, leaf first, whenever no automatic
 * transition is enabled: after the event was processed and again after each transition.
 *
 * \param sm      Machine.
 * \param state   State being run.
 * \param event   Event passed to hsm4c_run().
 *
 * \return        State to trigger an immediate external transition to. HSM4C_NO_STATE, the state
 *                itself or the current leaf state to not change state.
 *
 * \attention Transitions triggered by run functions are not visible in the transition
 *            tables. Be careful with this.
 */
typedef hsm4c_state_id_t (*hsm4c_run_fn_t)(hsm4c_t *sm, hsm4c_state_id_t state,
                                           hsm4c_event_id_t event);

/**
 * \brief Exit function prototype.
 *
 * \param sm      Machine.
 * \param state   State being exited.
 */
typedef void (*hsm4c_exit_fn_t)(hsm4c_t *sm, hsm4c_state_id_t state);

/**
 * \brief Transition action prototype.
 *
 * \param sm      Machine.
 */
typedef void (*hsm4c_action_fn_t)(hsm4c_t *sm);

/**
 * \brief Guard prototype.
 *
 * \param sm      Machine.
 *
 * \return        true if the transition should be taken, false otherwise.
 */
typedef bool (*hsm4c_guard_fn_t)(hsm4c_t const *sm);

/**
 * \brief Transition table row. The source is the state owning the table.
 *
 * Positional initializers read as "on event, go to target, if guard, do action".
 */
struct hsm4c_transition {
  /** \brief Event the transition reacts to. Must be positive or HSM4C_NO_EVENT. */
  hsm4c_event_id_t event;
  /** \brief Target state. HSM4C_NO_STATE for an internal (targetless) transition. */
  hsm4c_state_id_t to;
  /** \brief Guard. Called when the event matches. NULL means always true. (optional) */
  hsm4c_guard_fn_t guard_fn;
  /** \brief Action. Called after all exits, before all entries. (optional) */
  hsm4c_action_fn_t action_fn;
  /** \brief Transition type. Default: HSM4C_TRANSITION_EXTERNAL. */
  hsm4c_transition_type_t type;
};

/** \brief Use this as last element of a transition table. */
#define HSM4C_TRANSITIONS_END                                                                      \
  { .type = HSM4C_TRANSITION_TABLE_END }

/** \brief Constant state configuration. Element of a chart's state array. */
struct hsm4c_state {
  /** \brief Name of the state. Not used by the library. (optional) */
  char const *name;
  /** \brief Parent state. Ignored for the root. Default: HSM4C_ROOT. */
  hsm4c_state_id_t parent;
  /**
   * \brief Initial child state. Entered when this state is a transition target. (optional)
   *
   * For history states: the state entered when there is no history yet.
   */
  hsm4c_state_id_t initial;
  /** \brief State type. See hsm4c_state_type_t. Default: HSM4C_STATE_NORMAL. */
  hsm4c_state_type_t type;
  /** \brief Entry function. Called after the exits and the transition action. (optional) */
  hsm4c_entry_fn_t entry_fn;
  /** \brief Run function. Can trigger a transition. See hsm4c_run_fn_t. (optional) */
  hsm4c_run_fn_t run_fn;
  /** \brief Exit function. Called after the guard, before the transition action. (optional) */
  hsm4c_exit_fn_t exit_fn;
  /**
   * \brief Transition table, terminated by HSM4C_TRANSITIONS_END. (optional)
   *
   * The states of the active branch are searched from the leaf up to the root, so transitions
   * of deeper states take priority. Within a table, the first enabled row is taken.
   */
  hsm4c_transition_t const *transitions;
};

/** \brief Statechart: constant array of states, indexed by state ID. */
struct hsm4c_chart {
  /** \brief States. states[HSM4C_ROOT] is the root. */
  hsm4c_state_t const *states;
  /** \brief Number of states. */
  hsm4c_state_id_t num_states;
};

/** \brief Defines a chart from an array of states, e.g. `hsm4c_chart_t c = HSM4C_CHART(s);` */
#define HSM4C_CHART(states_array)                                                                  \
  { (states_array), (hsm4c_state_id_t)(sizeof(states_array) / sizeof((states_array)[0])) }

/** \brief Machine running a chart. All members are private, use the functions below. */
struct hsm4c {
  /** \brief Private. The chart. */
  hsm4c_chart_t const *chart;
  /** \brief Private. Active (or last active, for history) child per state. num_states entries. */
  hsm4c_state_id_t *active;
  /** \brief Private. Active leaf state. */
  hsm4c_state_id_t leaf;
  /** \brief Private. User context, see hsm4c_ctx(). */
  void *ctx;
};

/**
 * \brief Initializes a machine and starts the chart.
 *
 * Enters the root and its initial states down to a leaf, calling their entry functions,
 * then takes automatic transitions until none is enabled. Run functions are not called.
 * Calling it again restarts the chart and clears all history.
 *
 * \param sm      Machine to initialize.
 * \param chart   Chart to run. Must outlive the machine.
 * \param active  Runtime storage of `chart->num_states` elements. Must outlive the machine.
 * \param ctx     User context for the callbacks, see hsm4c_ctx(). Can be NULL.
 *
 * \return        Active leaf state after init.
 */
hsm4c_state_id_t hsm4c_init(hsm4c_t *sm, hsm4c_chart_t const *chart, hsm4c_state_id_t *active,
                            void *ctx);

/**
 * \brief Processes one event.
 *
 * Takes the enabled transition for the event, if any. Then takes automatic transitions
 * until none is enabled. Whenever no automatic transition is enabled, the run functions of
 * the active branch are called, and can request a further transition.
 *
 * \param sm      Machine.
 * \param event   Event to process. Must be > 0, or HSM4C_NO_EVENT to only poll.
 *
 * \return        Active leaf state after processing.
 */
hsm4c_state_id_t hsm4c_run(hsm4c_t *sm, hsm4c_event_id_t event);

/**
 * \brief Gets the user context given to hsm4c_init().
 *
 * Use it in callbacks to reach application data.
 *
 * \param sm      Machine.
 *
 * \return        User context.
 */
void *hsm4c_ctx(hsm4c_t const *sm);

/**
 * \brief Gets the active leaf state.
 *
 * \param sm      Machine.
 *
 * \return        Active leaf state.
 */
hsm4c_state_id_t hsm4c_current(hsm4c_t const *sm);

/**
 * \brief Checks whether a state is active, i.e. the active leaf or one of its ancestors.
 *
 * \param sm      Machine.
 * \param state   State to check.
 *
 * \return        true if the state is active.
 */
bool hsm4c_is_active(hsm4c_t const *sm, hsm4c_state_id_t state);

#ifdef __cplusplus
}
#endif
