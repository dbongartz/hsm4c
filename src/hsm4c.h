/** HSM4C - Statechart implementation in C
 *
 * \file
 *
 * - Fully hierachical statecharts.
 * - Entry and Exit state actions.
 * - Transition actions.
 * - Guard conditions.
 * - External and internal (local) transitions.
 * - Automatic transitions. (WIP)
 * - Transitions with optional shallow and deep history.
 * - Relatively easy table based syntax.
 * - No third-party dependencies.
 * - Small RAM footprint per state (1 or 2 pointers).
 * - Configuration & transitions can be const in FLASH.
 * - Only use what you need. All features except basic state and transitions can be disabled to save
 * memory.
 *
 * (C) 2024 David Bongartz
 * MIT License
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HSM4C_CONFIG_EVENT_ID_TYPE
  /** The type used for the event id. */
  #define HSM4C_CONFIG_EVENT_ID_TYPE int32_t
#endif

#ifndef HSM4C_CONFIG_NAME
  /** Enable the name field in `hsm4c_cfg_t`. */
  #define HSM4C_CONFIG_NAME 1
#endif

#ifndef HSM4C_CONFIG_STATE_USER_DATA
  /** Allow storage of a user data pointer in `hsm4c_state_t`. */
  #define HSM4C_CONFIG_STATE_USER_DATA 1
#endif

#ifndef HSM4C_CONFIG_EVENT_USER_DATA
  /** Allow storage of a user data pointer in `hsm4c_event_t`. */
  #define HSM4C_CONFIG_EVENT_USER_DATA 1
#endif

#ifndef HSM4C_CONFIG_TRANSITION_USER_DATA
  /** Allow storage of a user data pointer in `hsm4c_transition_t`. */
  #define HSM4C_CONFIG_TRANSITION_USER_DATA 1
#endif

#ifndef HSM4C_CONFIG_TRANSITION_GUARDS
  /** Allow a `guard_fn` per transition. */
  #define HSM4C_CONFIG_TRANSITION_GUARDS 1
#endif

#ifndef HSM4C_CONFIG_INTERNAL_TRANSITIONS
  /** Allow for internal transitions to self, parent or child. */
  #define HSM4C_CONFIG_INTERNAL_TRANSITIONS 1
#endif

#ifndef HSM4C_CONFIG_AUTOMATIC_TRANSITIONS
  /** Allow automaitc transitions after entering a state. */
  #define HSM4C_CONFIG_AUTOMATIC_TRANSITIONS 1
#endif

#ifndef HSM4C_CONFIG_HIERARCHICAL
  /** Enable hierachical states. */
  #define HSM4C_CONFIG_HIERARCHICAL 1
#endif

#if !HSM4C_CONFIG_HIERARCHICAL
  #define HSM4C_CONFIG_HISTORY_STATES 0
#endif

#ifndef HSM4C_CONFIG_HISTORY_STATES
  /** Allow history states. Only when `HSM4C_CONFIG_HIERARCHICAL=1`. */
  #define HSM4C_CONFIG_HISTORY_STATES 1
#endif

/** `HSM4C_CONFIG_ACTIONS` can enable or disable all action functions at once. */
#if defined(HSM4C_CONFIG_ACTIONS)
  #if HSM4C_CONFIG_ACTIONS
    #define HSM4C_CONFIG_ENTRY_FN 1
    #define HSM4C_CONFIG_EXIT_FN 1
    #define HSM4C_CONFIG_TRANSITION_FN 1
  #else
    #define HSM4C_CONFIG_ENTRY_FN 0
    #define HSM4C_CONFIG_EXIT_FN 0
    #define HSM4C_CONFIG_TRANSITION_FN 0
  #endif
#endif

#ifndef HSM4C_CONFIG_ENTRY_FN
  /** Allow a `entry_fn` per state. */
  #define HSM4C_CONFIG_ENTRY_FN 1
#endif

#ifndef HSM4C_CONFIG_EXIT_FN
  /** Allow a `exit_fn` per state. */
  #define HSM4C_CONFIG_EXIT_FN 1
#endif

#ifndef HSM4C_CONFIG_TRANSITION_FN
  /** Allow a `transition_fn` per transition. */
  #define HSM4C_CONFIG_TRANSITION_FN 1
#endif

#ifndef HSM4C_CONFIG_LOG
  #define HSM4C_CONFIG_LOG 1
#endif

#ifndef HSM4C_CONFIG_ASSERT
  #define HSM4C_CONFIG_ASSERT 1
#endif

#ifndef ARRAY_SIZE
  /** Convenience macro to determine size of an array in case it is not available. */
  #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/** Represents a state. */
typedef struct hsm4c_state hsm4c_state_t;

/** Represents the configuration for a state. Can be `const`. */
typedef struct hsm4c_state_cfg hsm4c_state_cfg_t;

/** Represents a single transition. Can be `const`. */
typedef struct hsm4c_transition hsm4c_transition_t;

/** Represents the target of a transition. */
typedef struct hsm4c_target_state hsm4c_target_state_t;

/** Represents an event. */
typedef struct hsm4c_event hsm4c_event_t;

/** User customizable event id type. See `HSM4C_CONFIG_EVENT_ID_TYPE`. */
typedef HSM4C_CONFIG_EVENT_ID_TYPE hsm4c_event_id_t;

#if HSM4C_CONFIG_ENTRY_FN
/** Entry action.
 * @param state state being entered.
 */
typedef void (*hsm4c_entry_fn)(hsm4c_state_t *state);
#endif

#if HSM4C_CONFIG_EXIT_FN
/** Exit action.
 * @param state state being exited.
 */
typedef void (*hsm4c_exit_fn)(hsm4c_state_t *state);
#endif

#if HSM4C_CONFIG_TRANSITION_FN
/** Transition action.
 * @param event event triggering the transition.
 */
typedef void (*hsm4c_transition_fn)(hsm4c_event_t event);
#endif

#if HSM4C_CONFIG_TRANSITION_GUARDS
/** Guard condition for transition.
 * @param event event triggering the transition.
 * @retval true if transition should be taken.
 * @retval false if transition should not be taken.
 */
typedef bool (*hsm4c_guard_fn)(hsm4c_event_t event);
#endif

#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
/** Transition type */
typedef enum hsm4c_transition_type {
  /** External transition. Calls entry and exit of source and target. (default) */
  HSM4C_TRANSITION_EXTERNAL = 0,
  /** Internal transition.
   *
   * Target => Self: Does not call entry or exit.
   * Target => Parent: Does call exit of self only.
   * Target => Child: Does call entry of child only.
   */
  HSM4C_TRANSITION_INTERNAL,
} hsm4c_transition_type_e;
#endif

#if HSM4C_CONFIG_HISTORY_STATES
/** History type */
typedef enum hsm4c_history {
  /** No history. Current states child and grandchildren will reset to `initial`. */
  HSM4C_HISTORY_NONE,
  /** Shallow history. Current states child will not reseit to `initial`. */
  HSM4C_HISTORY_SHALLOW,
  /** Deep history. Current states child and grandchildren will NOT reset to `initial`. */
  HSM4C_HISTORY_DEEP,
} hsm4c_history_e;
#endif

#if HSM4C_CONFIG_AUTOMATIC_TRANSITIONS
/** Take this transition immediately after entering the state. */
  #define HSM4C_E_AUTOMATIC (-1)
#endif

struct hsm4c_state_cfg {
  /** Array of transitions for this state. Can be NULL.*/
  hsm4c_transition_t const *transitions;
  /** Number of transitions in `transitions`. Can be 0. */
  size_t num_transitions;

#if HSM4C_CONFIG_NAME
  /** Name of state. Can be `NULL` */
  char const *name;
#endif

#if HSM4C_CONFIG_ENTRY_FN
  /** Entry action. Can be `NULL`. */
  hsm4c_entry_fn entry_fn;
#endif

#if HSM4C_CONFIG_EXIT_FN
  /** Exit action. Can be `NULL`. */
  hsm4c_exit_fn exit_fn;
#endif

#if HSM4C_CONFIG_HIERARCHICAL
  /** Parent state. Set to `NULL` if this is a root state. */
  hsm4c_state_t *parent;
  /** Initial child state. *MUST* be set if this state has children. */
  hsm4c_state_t *initial;
#endif
};

struct hsm4c_event {
  /** The event id. Must be >= 0. */
  hsm4c_event_id_t id;

#if HSM4C_CONFIG_EVENT_USER_DATA
  /** User data pointer. Not accessed by library. */
  void *data;
#endif
};

struct hsm4c_target_state {
  /** Target state. Cannot be `NULL` */
  hsm4c_state_t *state;

#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
  /** Transition type. 0 = External. */
  hsm4c_transition_type_e transition_type;
#endif

#if HSM4C_CONFIG_HISTORY_STATES
  /** Controls if the target should be entered with history enabled. */
  hsm4c_history_e history;
#endif

#if HSM4C_CONFIG_STATE_USER_DATA
  /** User data pointer. Not accessed by library. */
  void *data;
#endif
};

struct hsm4c_transition {
  /** Event id for this transition. */
  hsm4c_event_id_t event_id;

  /** Target of transition. */
  hsm4c_target_state_t target;

#if HSM4C_CONFIG_TRANSITION_GUARDS
  /** Guard condition. Can be NULL. */
  hsm4c_guard_fn guard_fn;
#endif

#if HSM4C_CONFIG_TRANSITION_FN
  /** Transition action. Can be NULL. */
  hsm4c_transition_fn transition_fn;
#endif

#if HSM4C_CONFIG_TRANSITION_USER_DATA
  /** User data pointer. Not accessed by library. */
  void *data;
#endif
};

struct hsm4c_state {
  /** State configuration. Cannot be `NULL`. */
  struct hsm4c_state_cfg const *cfg;
#if HSM4C_CONFIG_HIERARCHICAL
  /** Private: Current active child. */
  struct hsm4c_state *active_child;
#endif
};

/** Initialize the statemachine to this state.
 *
 * Does only initialize and call all `entry_fn` of the branch this state is on.
 * Does NOT reset the full statemachine as it can be costly.
 * To do so manually set all `hsm4c_state::active_child` to `NULL`.
 *
 * @param initial The statemachines initial state. Can be a child.
 * @return        The resulting current (leaf) state.
 */
hsm4c_state_t *hsm4c_init(hsm4c_state_t *initial);

/** Dispatch a event to the statemachine.
 *
 * @param current The current (leaf) state.
 * @param e       The event to dispatch.
 * @return        The new (leaf) state.
 */
hsm4c_state_t *hsm4c_dispatch(hsm4c_state_t *current, hsm4c_event_t e);

/** Get the name of a state if available
 *
 * @param state State to get name of if enabled & set.
 * @return      Name of state or empty string if not enabled or set.
 */
char const *hsm4c_get_name(hsm4c_state_t const *state);

#ifdef __cplusplus
}
#endif
