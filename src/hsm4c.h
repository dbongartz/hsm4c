/**
 * \brief Statechart implementation in C
 * \file
 *
 * Tries to make UML2 statecharts accessible in C.
 *
 * Supports:
 *
 * - Fully hierachical statecharts.
 * - Entry/Exit/Run state actions.
 * - Run can trigger immediate transition.
 * - Transitions with guard and action.
 * - Internal (Local) and external transitions.
 * - Automatic transitions.
 * - Initial child states.
 * - History and Deep History pseudo states with initial state.
 * - Choice pseudo states.
 * - Relatively easy table based syntax. (See tests).
 *
 * Does not support:
 * - Parallel states
 *
 * (C) 2023 David Bongartz
 * MIT License
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct hsm4c_state hsm4c_state_t;
typedef struct hsm4c_state_config hsm4c_state_config_t;
typedef struct hsm4c_transition hsm4c_transition_t;
typedef int hsm4c_event_t;

/** \brief hsm4c_State Types */
typedef enum hsm4c_state_type {
  /** \brief Default (compund) state type */
  HSM4C_TYPE_NORMAL = 0,

  /**
   * \brief History pseudo state. Must not have entry/exit/run/children.
   *
   * Parent states last active substate will be entered and reset.
   * If parent does not have an active substate, will try using the
   * specified initial state if present, the parents initial state otherwise.
   */
  HSM4C_TYPE_HISTORY,

  /**
   * \brief Deep History pseudo state. Must not have entry/exit/run/children.
   *
   * Parent states last active substate and children of that one will be entered
   * and NOT reset.
   *
   * If parent does not have an active substate, will try using the
   * specified initial state if present, the parents initial state otherwise.
   */
  HSM4C_TYPE_HISTORY_DEEP,

  /**
   * \brief Choice pseudo state.
   *
   * Must not have entry/exit/run/children.
   * Must only have automatic (HSM4C_NO_EVENT) guarded transition. Transitions are evaluated in
   * table order.
   */
  HSM4C_TYPE_CHOICE,

  /**
   * \brief Root pseudo state.
   *
   * Can have entry/exit/run functions.
   * Must have initial state.
   * Must have no parent (NULL).
   */
  HSM4C_TYPE_ROOT,
} hsm4c_state_type_e;

/** \brief hsm4c_Transition Types */
typedef enum hsm4c_transition_type {
  /** \brief External (default) transition. Source and target states are exited and entered. */
  HSM4C_TTYPE_EXTERNAL = 0,

  /**
   * \brief Local transition. Source is not exited.
   *
   * Must be "Parent -> Child" or "Same -> Same"
   * When "Same -> Same" no entry and exit function is executed (internal transition)
   */
  HSM4C_TTYPE_LOCAL,

  /**
   * \brief Use to indicate the end of transition tables
   */
  HSM4C_TTYPE_TABLE_END,
} hsm4c_transition_type_e;

/** \brief Special events. Must be <= 0 */
typedef enum hsm4c_events {
  /** \brief Use this for event-less / automatic transitions. */
  HSM4C_NO_EVENT = -1,
} hsm4c_events_e;

/**
 * \brief Entry function prototype.
 *
 * \param s   Current state
 */
typedef void (*hsm4c_entry_fn)(hsm4c_state_t const *s);

/**
 * \brief Run function prototype.
 *
 * \param s   Current state.
 * \param e   Event of transition if any. HSM4C_NO_EVENT if not in transition.
 *
 * \return    Valid state to trigger immediate transition. NULL to not change state.
 *
 * \attention Transitions from here are not visible in the transition table.
 *            Be careful with this.
 */
typedef hsm4c_state_t *(*hsm4c_run_fn)(hsm4c_state_t const *s, hsm4c_event_t e);

/**
 * \brief Exit function prototype.
 *
 * \param s   Current state
 */
typedef void (*hsm4c_exit_fn)(hsm4c_state_t const *s);

/**
 * \brief hsm4c_Transition action prototype.
 *
 * \param s   Root state
 */
typedef void (*hsm4c_transition_fn)(hsm4c_state_t const *root);

/**
 * \brief Guard prototype.
 *
 * \param s   Root state
 *
 * \return    true if transition should be taken. false otherwise.
 */
typedef bool (*hsm4c_guard_fn)(hsm4c_state_t const *root);

/** \brief hsm4c_Transition class */
struct hsm4c_transition {
  /** \brief Source state of transition. Must be a valid state. */
  hsm4c_state_t *const from;
  /** \brief Target state of transition. Must be a valid state. */
  hsm4c_state_t *const to;
  /** \brief Event the transition reacts to. Must be positive or one of hsm4c_ScEvents */
  hsm4c_event_t const event;
  /** \brief hsm4c_Transition function. Will be called after all exits, before all entrys */
  hsm4c_transition_fn const transition_fn;
  /** \brief Guard. Return true to take transition. Called when matching source and event found */
  hsm4c_guard_fn const guard_fn;
  /** \brief hsm4c_Transition type. Default: External */
  hsm4c_transition_type_e type;
};

/** \brief Use this to indicate the end of the transition table. */
#define HSM4C_TRANSITIONS_END ((hsm4c_transition_t const){.type = HSM4C_TTYPE_TABLE_END})

struct hsm4c_state_config {
  /** \brief Name of the state (optional) */
  char const *name;
  /** \brief Entry function. Will be called after transition guard. (optional) */
  hsm4c_entry_fn const entry_fn;
  /** \brief Run function. Can change state. After transition or when no transition (optional) */
  hsm4c_run_fn const run_fn;
  /** \brief Entry function. After transition guard. (optional) */
  hsm4c_exit_fn const exit_fn;
  /** \brief Parent state. Must be statchart root or NULL if this is root state. (mandatory) */
  hsm4c_state_t *const parent;
  /** \brief Initial state. When target is this state, also transition into initial. (optional) */
  hsm4c_state_t *const initial;
  /** \brief hsm4c_State type. See hsm4c_StateType description. (optional) */
  hsm4c_state_type_e type;
  /** \brief hsm4c_State transition table. Only used on root node currently. */
  hsm4c_transition_t const *transitions;
};

/** \brief hsm4c_State class */
struct hsm4c_state {
  hsm4c_state_config_t const *config;

  /** \brief Active child state. On root node this is always a leaf */
  hsm4c_state_t *_active;
};

/**
 * \brief Initialized a statechart
 *
 * This does only initialize the root tree.
 * To reset all states please iterate with `hsm4c_reset_state()`.
 *
 * \param root          Statechart root state.
 * \param transitions   hsm4c_Transition table. Last element must be HSM4C_TRANSITIONS_END.
 *
 * \return              Leaf state after init.
 */
hsm4c_state_t const *hsm4c_init(hsm4c_state_t *root);

/** \brief Resets the given state */
void hsm4c_reset_state(hsm4c_state_t *state);

/** \brief Map StateConfigs and hsm4c_State if using tables to define them */
void hsm4c_assign_stateconfigs_to_states(size_t num_states, hsm4c_state_t *states,
                                         hsm4c_state_config_t const *statecfgs);

/**
 * \brief Runs one iteration of the statechart
 *
 * \param root    Statechart root state.
 * \param event   Event to pass to the statechart.
 *                Events <= 0 are used internally.
 *                E.g. HSM4C_NO_EVENT is 0
 *
 * \return        hsm4c_State after one iteration.
 */
hsm4c_state_t const *hsm4c_run(hsm4c_state_t *root, hsm4c_event_t event);

/**
 * \brief Get the root of any state
 *
 * \param s       hsm4c_State to search from.
 *
 * \return        Root state.
 */
hsm4c_state_t const *hsm4c_get_root(hsm4c_state_t const *s);

#ifdef __cplusplus
}
#endif
