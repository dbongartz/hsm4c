// #pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------- Config -------- */

#ifndef HSM4C_CONFIG_STATE_NAME
  /** Enable the name field in `hsm4c_state_t`. */
  #define HSM4C_CONFIG_STATE_NAME 1
#endif

#ifndef HSM4C_CONFIG_TRIGGER_NAME
  /** Enable the name field in `hsm4c_trigger_t`. */
  #define HSM4C_CONFIG_TRIGGER_NAME 1
#endif

#ifndef HSM4C_CONFIG_STATE_CTX
  /** Enable the ctx field in `hsm4c_state_t`. */
  #define HSM4C_CONFIG_STATE_CTX 1
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
  #define HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE 0
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
  #define ARRAY_SIZE(_array) (sizeof((_array)) / sizeof((_array)[0]))
#endif

#ifndef HSM4C_ASSERT
  #ifdef NDEBUG
    #define HSM4C_ASSERT(...) ((void)0)
  #else
    #define HSM4C_ASSERT(...) assert(__VA_ARGS__)
  #endif
#endif

/* -------- Definitions -------- */

enum { HSM4C_STATE_ID_ZERO = 0 };

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
  char const *name;
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
typedef bool (*guard_fn_t)(void *ctx, hsm4c_trigger_t trigger);
#endif

#if HSM4C_CONFIG_TRANSITION_FN
typedef void (*transition_fn_t)(void *ctx, hsm4c_trigger_t trigger);
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

typedef struct hsm4c_transition_table {
  hsm4c_transition_t const *entries;
  size_t num;
} hsm4c_transition_table_t;

/* -------- States -------- */

#if HSM4C_CONFIG_STATE_NAME
  #define HSM4C_STATE_ROOT(inital) {.compound.initial = 0, .name = "ROOT"}
#else
  #define HSM4C_STATE_ROOT(inital) {.compound.initial = 0}
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
typedef void (*entry_fn_t)(void *ctx, hsm4c_state_t const *state);
#endif

#if HSM4C_CONFIG_EXIT_FN
typedef void (*exit_fn_t)(void *ctx, hsm4c_state_t const *state);
#endif

typedef hsm4c_state_id_t (*choice_fn_t)(void *ctx, hsm4c_state_id_t state);

typedef struct hsm4c_state_compound {
  hsm4c_state_id_t initial;

#if HSM4C_CONFIG_ENTRY_FN
  entry_fn_t entry_fn;
#endif

#if HSM4C_CONFIG_EXIT_FN
  exit_fn_t exit_fn;
#endif

#if !HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  hsm4c_transition_table_t transitions;
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
  char const *name;
#endif
#if HSM4C_CONFIG_STATE_CTX
  void *ctx;
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
  hsm4c_transition_table_t transitions;
#endif
  void *ctx;
} hsm4c_cfg_t;

typedef struct hsm4c_sm {
  hsm4c_cfg_t const *cfg;
  hsm4c_state_id_t active_state;
} hsm4c_t;

typedef enum {
  HSM4C_OK,
  HSM4C_DEFERED,
  HSM4C_ERROR,
} hsm4c_result_e;

/* -------- API -------- */

hsm4c_result_e hsm4c_start(hsm4c_t *self, hsm4c_cfg_t const *cfg);
hsm4c_result_e hsm4c_run2completion(hsm4c_t *self, hsm4c_trigger_t trigger);

/* -------- Utilities -------- */

hsm4c_state_t const *hsm4c_get_current_state(hsm4c_t const *self);
hsm4c_transition_t const *hsm4c_find_transition(hsm4c_t const *self, hsm4c_trigger_t trigger);
hsm4c_state_id_t hsm4c_get_parent_state_id(hsm4c_t const *self, hsm4c_state_id_t state_id);

void hsm4c_print_current_state_branch(hsm4c_t const *self);
void hsm4c_print_transition(hsm4c_t const *self, hsm4c_transition_t const *transition);

#ifdef __cplusplus
}
#endif
