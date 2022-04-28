#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

typedef struct state_m state_m_t;
typedef struct state state_t;
typedef struct tran tran_t;

struct state_m {
    state_t *state;
    void *data;
};

struct state {
    const char * name;
    void (*entry_fn)(void *data);
    void (*exit_fn)(void *data);
    tran_t *tran_list;
    size_t tran_list_size;
    state_t *child;
    state_t *parent;
    state_t *initial;
};

struct tran {
    int event_n;
    void (*action_fn)(void *data);
    bool (*guard_fn)(void *data);
    state_t *target_state;
};

#define STATE_NAME(name) state_##name

#define DEFINE_STATE(name) static state_t STATE_NAME(name);

#define TRAN(event, action, guard, state) { \
    .event_n = event, \
    .action_fn = action, \
    .guard_fn = guard, \
    .target_state = &STATE_NAME(state), \
}

#define POPULATE_STATE(staten, entry, exit, tran, ...) \
static tran_t tran_list_##staten[] = { tran, __VA_ARGS__ }; \
static state_t STATE_NAME(staten) = { \
    .name = #staten, \
    .tran_list = tran_list_##staten, \
    .tran_list_size = sizeof(tran_list_##staten) / sizeof(tran_list_##staten[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
}

void dispatch(state_m_t *statem, int event);
bool dispatch_hsm(state_t *s, int event);

void test(state_m_t *state_m);

#endif