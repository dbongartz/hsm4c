#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

/* -------- Typedefs -------- */

typedef struct event event_t;
typedef int event_num_t;

typedef void (*action_fn_t)(void *data);
typedef bool (*guard_fn_t)(event_t e);
typedef void (*transition_fn_t)(event_t e);
typedef void (*entry_fn_t)(void *data);
typedef void (*exit_fn_t)(void *data);
typedef enum s_flags s_flags_t;

/* --------- Data structures -------- */

typedef struct state_m state_m_t;
typedef struct state state_t;
typedef struct tran tran_t;
typedef struct const_state const_state_t;

struct state_m {
    state_t *state;
    void *data;
};

enum s_flags {
    STATE_FLAG_NONE = 0,
    STATE_FLAG_HISTORY = (1 << 0),
    STATE_FLAG_END = (1 << 1),
};

struct event {
    event_num_t num;
    void *data;
};

struct const_state {
    char const * const name;
    entry_fn_t entry_fn;
    exit_fn_t exit_fn;
    tran_t const * const tran_list;
    const size_t tran_list_size;
    state_t * const parent;
    state_t * const initial;
    action_fn_t initial_fn;
    s_flags_t flags;
};

struct state {
    state_t *child;
    const const_state_t *_state;
};

struct tran {
    event_num_t event_n;
    transition_fn_t action_fn;
    guard_fn_t guard_fn;
    state_t *target_state;
};

/* --------- Private ---------- */

#define _STATE_NAME(name) state_##name
#define _TRAN_LIST_NAME(name) name##_tran_list
#define _CONST_STATE_NAME(name) name##_const_state

/* Hack to work around having to enter a `&state_name` on POPULATE_STATE(...)
 * Works by checking the size of state_NONE which is an empty struct versus the actual struct
 * Never reference state_NONE please :)
 */
extern struct{} state_NONE;
#define _STATE_T_PTR_OR_NULL(name) sizeof(state_t) == sizeof(_STATE_NAME(name)) ? \
    (state_t *)&_STATE_NAME(name) : NULL


#define _DEFINE_STATE(name, initial_state) \
state_t _STATE_NAME(name) = { \
    .child = _STATE_T_PTR_OR_NULL(initial_state), \
    ._state = &_CONST_STATE_NAME(name) \
}

#define _POPULATE_STATE(_name, _entry_fn, _exit_fn, parent_state, initial_state, initial_action, _flags, ...) \
static const tran_t _TRAN_LIST_NAME(_name)[] = { __VA_ARGS__ }; \
static const const_state_t _CONST_STATE_NAME(_name) = { \
    .name = #_name, \
    .tran_list = _TRAN_LIST_NAME(_name), \
    .tran_list_size = sizeof(_TRAN_LIST_NAME(_name)) / sizeof(_TRAN_LIST_NAME(_name)[0]), \
    .entry_fn = _entry_fn, \
    .exit_fn = _exit_fn, \
    .parent = _STATE_T_PTR_OR_NULL(parent_state), \
    .initial = _STATE_T_PTR_OR_NULL(initial_state), \
    .initial_fn = initial_action, \
    .flags = _flags \
};

/* -------- Public defines -------- */

#define STATE_DEF(name) _STATE_NAME(name)
#define STATE_REF(name) &_STATE_NAME(name)

#define DECLARE_STATE(name) static state_t _STATE_NAME(name);
#define DECLARE_STATE_GLOBAL(name) DECLARE_STATE(name);

#define POPULATE_STATE(name, entry_fn, exit_fn, parent_state, initial_state, initial_fn, flags, ...) \
_POPULATE_STATE(name, entry_fn, exit_fn, parent_state, initial_state, initial_fn, flags, __VA_ARGS__) \
static _DEFINE_STATE(name, initial_state)

#define POPULATE_STATE_GLOBAL(name, entry_fn, exit_fn, parent_state, initial_state, initial_fn, flags, ...) \
_POPULATE_STATE(name, entry_fn, exit_fn, parent_state, initial_state, initial_fn, flags, __VA_ARGS__) \
_DEFINE_STATE(name, initial_state)

#define TRAN(event, action, guard, target) { \
    .event_n = event, \
    .action_fn = action, \
    .guard_fn = guard, \
    .target_state = _STATE_T_PTR_OR_NULL(target), \
}

#define STATE_SELF NONE

/* -------- Public methods --------- */

/**
 * @brief Dispatch event to a root statemachine
 *
 * @param statem root statemachine
 * @param event event to post
 */
void dispatch(state_m_t *statem, int event);
bool dispatch_hsm(state_t *s, event_num_t event_num, void *user_data);

state_t *get_state(state_t *sm);
state_t *get_statemachine(state_t *s);
void reset_statemachine(state_t *sm);

#endif