#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

typedef struct state_m state_m_t;
typedef struct state state_t;
typedef struct tran tran_t;

struct state_m {
    const state_t *state;
    void *data;
};

struct state {
    void (*entry_fn)(void *data);
    void (*exit_fn)(void *data);
    const tran_t *tran_list;
    const size_t tran_list_size;
};

struct tran {
    const int event_n;
    void (*action_fn)(void *data);
    bool (*guard_fn)(void *data);
    const state_t *target_state;
};

#define STATE(name) static const state_t state_##name

#define TRAN(event, action, guard, state) { .event_n = event, .action_fn = action, .guard_fn = guard, .target_state = &state_##state }

#define ASSIGN_TRAN(name, entry, exit, user_data, ...) \
static const tran_t tran_list_##name[] = { __VA_ARGS__ }; \
static const state_t state_##name = { \
    .tran_list = tran_list_##name, \
    .tran_list_size = sizeof(tran_list_##name) / sizeof(tran_list_##name[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
}

void dispatch(state_m_t *statem, int event);

void test(state_m_t *state_m);

#endif