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

#define STATE_NAME(name) name

#define DECLARE_STATE(name) const state_t STATE_NAME(name);

#define TRAN(event, action, guard, state_ptr) { \
    event, action, guard, state_ptr }

#define TRAN_INTERNAL(event, action, guard) TRAN(event, action, guard, NULL)

#define POPULATE_STATE(state_name, entry, exit, tran, ...) \
const tran_t tran_list_##state_name[] = { tran, __VA_ARGS__ }; \
const state_t state_name = { \
    .name = #state_name, \
    .tran_list = tran_list_##state_name, \
    .tran_list_size = sizeof(tran_list_##state_name) / sizeof(tran_list_##state_name[0]), \
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