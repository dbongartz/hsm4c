#ifndef __HSM4C__H__
#define __HSM4C__H__

#include <stddef.h>
#include <stdbool.h>

typedef struct state_machine state_machine_t;
typedef struct state state_t;
typedef struct transition tran_t;

struct state_machine {
    const state_t *state;
    void *data;
};

struct state {
    char *name;
    void (*entry_fn)(void *data);
    void (*exit_fn)(void *data);
    const tran_t *tran_list;
    const size_t tran_list_size;
};

struct transition {
    const int event;
    void (*action_fn)(void *data);
    bool (*guard_fn)(void *data);
    const state_t *target_state;
};

#define STATE_NAME(name) state_##name

#define DEFINE_STATE(name) static const state_t STATE_NAME(name);

#define TRAN(event, action, guard, state) { \
    event, action, guard, &STATE_NAME(state) }

#define POPULATE_STATE(staten, entry, exit, tran, ...) \
static const tran_t tran_list_##staten[] = { tran, __VA_ARGS__ }; \
static const state_t STATE_NAME(staten) = { \
    .name = #staten, \
    .tran_list = tran_list_##staten, \
    .tran_list_size = sizeof(tran_list_##staten) / sizeof(tran_list_##staten[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
}

#define DEFINE_STATEM(name, initial_state, user_data_ptr) \
state_machine_t name = { .state = &STATE_NAME(initial_state), .data = user_data_ptr }


/**
 * @brief Dispatch an event to the statemachine
 *
 * The state machine will return after "Run to completion"
 *
 * @param statem statemachine pointer
 * @param event event to dispatch
 */
void dispatch(state_machine_t *statem, int event);

#endif