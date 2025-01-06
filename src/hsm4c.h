// #pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------- Config -------- */

#ifndef HSM4C_CONFIG_STATE_NAME
  /** Enable the name field in `hsm4c_state_t`. */
  #define HSM4C_CONFIG_STATE_NAME 0
#endif

#ifndef HSM4C_CONFIG_TRIGGER_NAME
  /** Enable the name field in `hsm4c_trigger_t`. */
  #define HSM4C_CONFIG_TRIGGER_NAME 0
#endif

#ifndef HSM4C_CONFIG_TRANSITION_GUARDS
  /** Allow a `guard_fn` per transition. */
  #define HSM4C_CONFIG_TRANSITION_GUARDS 1
#endif

#ifndef HSM4C_CONFIG_INTERNAL_TRANSITIONS
  /** Allow for internal transitions to self, parent or child. */
  #define HSM4C_CONFIG_INTERNAL_TRANSITIONS 1
#endif

#ifndef HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  #define HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE 1
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

/* -------- Utility -------- */

#ifndef ARRAY_SIZE
  #define ARRAY_SIZE(_array) sizeof((_array)) / sizeof((_array)[0])
#endif

#define HSM4C_STATE_ID_RESERVED (0)

typedef struct hsm4c_state hsm4c_state_t;
typedef uint32_t hsm4c_state_id_t;
typedef uint32_t hsm4c_size_t;
typedef uint32_t hsm4c_trigger_id_t;

/* -------- Trigger -------- */

typedef enum {
  HSM4C_TRIGGER_NORMAL,
  HSM4C_TRIGGER_COMPLETION_EVENT,
} hsm4c_trigger_variant_e;

typedef struct hsm4c_trigger_normal {
  hsm4c_trigger_id_t id;
#if HSM4C_CONFIG_TRIGGER_NAME
  char *const name;
#endif
  void *data;
} hsm4c_trigger_normal_t;

typedef struct hsm4c_trigger_completion_event {
  void *data;
} hsm4c_trigger_completion_event_t;

typedef struct hsm4c_trigger {
  hsm4c_trigger_variant_e variant;
  union {
    hsm4c_trigger_normal_t normal;
    hsm4c_trigger_completion_event_t completion_event;
  };
} hsm4c_trigger_t;

/* -------- Transitions -------- */

#if HSM4C_CONFIG_TRANSITION_GUARDS
typedef bool (*guard_fn_t)(void *ctx, hsm4c_trigger_t t);
#endif

#if HSM4C_CONFIG_TRANSITION_FN
typedef void (*transition_fn_t)(void *ctx, hsm4c_trigger_t t);
#endif

typedef struct hsm4c_transition {
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  hsm4c_state_id_t source;
#endif

  hsm4c_state_id_t target;

  hsm4c_trigger_t trigger;

#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
  bool internal;
#endif

#if HSM4C_CONFIG_TRANSITION_GUARDS
  guard_fn_t guard_fn;
#endif

#if HSM4C_CONFIG_TRANSITION_FN
  transition_fn_t transition_fn;
#endif

} hsm4c_transition_t;

/* -------- States -------- */

#if HSM4C_CONFIG_STATE_NAME
  #define HSM4C_STATE_ROOT(inital) {.compound.initial = S0, .name = "ROOT"}
#else
  #define HSM4C_STATE_ROOT(inital) {.compound.initial = S0}
#endif

typedef enum {
  HSM4C_STATE_COMPOUND,
  HSM4C_STATE_HISTORY,
  HSM4C_STATE_DEEP_HISTORY,
  HSM4C_STATE_FINAL,
  HSM4C_STATE_TERMINATION,
  HSM4C_STATE_CHOICE,
  HSM4C_STATE_ENTRY,
  HSM4C_STATE_EXIT,
} hsm4c_state_variant_e;

#if HSM4C_CONFIG_ENTRY_FN
typedef void (*entry_fn_t)(void *ctx, hsm4c_state_t const *s);
#endif

#if HSM4C_CONFIG_EXIT_FN
typedef void (*exit_fn_t)(void *ctx, hsm4c_state_t const *s);
#endif

typedef hsm4c_state_id_t (*choice_fn_t)(void *ctx, hsm4c_state_id_t s);

typedef struct hsm4c_state_compound {
  hsm4c_state_id_t initial;

#if HSM4C_CONFIG_ENTRY_FN
  entry_fn_t entry_fn;
#endif

#if HSM4C_CONFIG_EXIT_FN
  exit_fn_t exit_fn;
#endif

#if !HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  hsm4c_transition_t const *transitions;
  hsm4c_size_t transitions_num;
#endif
} hsm4c_state_compound_t;

// typedef struct hsm4c_state_history {
// } hsm4c_state_history_t;

// typedef struct hsm4c_state_deep_history {
// } hsm4c_state_deep_history_t;

// typedef struct hsm4c_state_final {
// } hsm4c_state_final_t;

// typedef struct hsm4c_state_termination {
// } hsm4c_state_termination_t;

typedef struct hsm4c_state_choice {
  choice_fn_t choice_fn;
} hsm4c_state_choice_t;

typedef struct hsm4c_state_entry {
  hsm4c_state_id_t target;
} hsm4c_state_entry_t;

typedef struct hsm4c_state_exit {
  hsm4c_state_id_t target;
} hsm4c_state_exit_t;

struct hsm4c_state {
  hsm4c_state_id_t parent;
  hsm4c_state_variant_e variant;
  union {
    hsm4c_state_compound_t compound;
    // hsm4c_state_history_t history;
    // hsm4c_state_deep_history_t deep_history;
    // hsm4c_state_final_t final;
    // hsm4c_state_termination_t termination;
    hsm4c_state_choice_t choice;
    hsm4c_state_entry_t entry;
    hsm4c_state_exit_t exit;
  };
#if HSM4C_CONFIG_STATE_NAME
  char *const name;
#endif
};

typedef struct hsm4c_state_compound_rt {
  hsm4c_state_id_t active_substate;
} hsm4c_state_compound_rt_t;

typedef struct hsm4c_state_rt {
  union {
    hsm4c_state_compound_rt_t compound;
  };
} hsm4c_state_rt_t;

/* -------- Statemachine -------- */

typedef struct hsm4c_cfg {
  hsm4c_state_t const *states;
  hsm4c_state_rt_t *states_rt;
  hsm4c_size_t num_states;
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  hsm4c_size_t transitions_num;
  hsm4c_transition_t const *transitions;
#endif
  void *ctx;
} hsm4c_cfg_t;

typedef struct hsm4c_sm {
  hsm4c_cfg_t const *cfg;
  hsm4c_state_id_t current_state;
} hsm4c_t;

typedef enum {
  HSM4C_OK,
  HSM4C_DEFERED,
  HSM4C_ERROR,
} hsm4c_result_e;

/* -------- Functions -------- */

hsm4c_result_e hsm4c_init(hsm4c_t *self, hsm4c_cfg_t const *cfg);
hsm4c_result_e hsm4c_run2completion(hsm4c_t *self, hsm4c_trigger_t trigger);
void hsm4c_print_states(hsm4c_t const *self);

#ifdef __cplusplus
}
#endif
