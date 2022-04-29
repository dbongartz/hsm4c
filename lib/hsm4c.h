#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

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
    void (*entry_fn)(void *data);
    void (*exit_fn)(void *data);
    const tran_t *tran_list;
    const size_t tran_list_size;
    state_t *parent;
    state_t *initial;
};

struct state {
    state_t *child;
    const_state_t *_state;
};

struct tran {
    int event_n;
    void (*action_fn)(void *data);
    bool (*guard_fn)(void *data);
    state_t *target_state;
};

#define STATE_NAME(name) state_##name

#define DEFINE_STATE(name) static state_t state_##name;

#define TRAN(event, action, guard, staten) { \
    .event_n = event, \
    .action_fn = action, \
    .guard_fn = guard, \
    .target_state = &state_##staten, \
}

#define POPULATE_STATE(staten, entry, exit, parent_state, initial_state, ...) \
static const tran_t tran_list_##staten[] = { __VA_ARGS__ }; \
static const const_state_t const_state##staten = { \
    .name = #staten, \
    .tran_list = tran_list_##staten, \
    .tran_list_size = sizeof(tran_list_##staten) / sizeof(tran_list_##staten[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
    .parent = parent_state, \
    .initial = initial_state \
}; \
static state_t STATE_NAME(staten) = { \
    .child = initial_state, \
    ._state = &const_state##staten \
}

void dispatch(state_m_t *statem, int event);
bool dispatch_hsm(state_t *s, int event);

void test(state_m_t *state_m);

#endif