#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

/* -------- Typedefs -------- */

typedef int event_t;

typedef void (*action_fn_t)(void *data);
typedef bool (*guard_fn_t)(void *data);
typedef action_fn_t entry_fn_t;
typedef action_fn_t exit_fn_t;


/* --------- Data structures -------- */

typedef struct state_m state_m_t;
typedef struct state state_t;
typedef struct tran tran_t;
typedef struct const_state const_state_t;

struct state_m {
    state_t *state;
    void *data;
};

struct const_state {
    const char * name;
    entry_fn_t entry_fn;
    exit_fn_t exit_fn;
    const tran_t *tran_list;
    const size_t tran_list_size;
    state_t *parent;
    state_t *initial;
};

struct state {
    state_t *child;
    const const_state_t *_state;
};

struct tran {
    event_t event_n;
    action_fn_t action_fn;
    guard_fn_t guard_fn;
    state_t *target_state;
};

/* --------- Private ---------- */

#define _STATE_NAME(name) state_##name

#define _DEFINE_STATE(staten, initial_state) \
state_t STATE_NAME(staten) = { \
    .child = initial_state, \
    ._state = &const_state##staten \
}

#define _POPULATE_STATE(staten, entry, exit, parent_state, initial_state, ...) \
static const tran_t tran_list_##staten[] = { __VA_ARGS__ }; \
static const const_state_t const_state##staten = { \
    .name = #staten, \
    .tran_list = tran_list_##staten, \
    .tran_list_size = sizeof(tran_list_##staten) / sizeof(tran_list_##staten[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
    .parent = parent_state, \
    .initial = initial_state \
};

/* -------- Public defines -------- */

#define STATE_NAME(name) _STATE_NAME(name)

#define DECLARE_STATE(name) static state_t state_##name;
#define DECLARE_STATE_GLOBAL(name) DECLARE_STATE(name);

#define POPULATE_STATE(staten, entry, exit, parent_state, initial_state, ...) \
_POPULATE_STATE(staten, entry, exit, parent_state, initial_state, __VA_ARGS__) \
static _DEFINE_STATE(staten, initial_state)

#define POPULATE_STATE_GLOBAL(staten, entry, exit, parent_state, initial_state, ...) \
_POPULATE_STATE(staten, entry, exit, parent_state, initial_state, __VA_ARGS__) \
_DEFINE_STATE(staten, initial_state)

#define TRAN(event, action, guard, staten) { \
    .event_n = event, \
    .action_fn = action, \
    .guard_fn = guard, \
    .target_state = &state_##staten, \
}

/* -------- Public methods --------- */

/**
 * @brief Dispatch event to a root statemachine
 *
 * @param statem root statemachine
 * @param event event to post
 */
void dispatch(state_m_t *statem, int event);
bool dispatch_hsm(state_t *s, int event);

state_t *get_state(state_t *sm);
state_t *get_statemachine(state_t *s);

#endif