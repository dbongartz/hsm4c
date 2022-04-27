#include <stddef.h>
#include <stdio.h>

struct state;
struct tran;

struct state {
    void (*entry_fn)(void);
    void (*exit_fn)(void);
    const struct tran *tran_list;
    const size_t tran_list_size;
};


struct tran {
    const int event_n;
    void (*action_fn)(void);
    const struct state *target_state;
};

struct state state_NULL;

#define STATE(name) \
static const struct state state_##name

#define TRAN(event, action, state) { .event_n = event, .action_fn = action, .target_state = &state_##state }

#define ASSIGN_TRAN(name, entry, exit, ...) \
static const struct tran tran_list_##name[] = { __VA_ARGS__ }; \
static const struct state state_##name = { \
    .tran_list = tran_list_##name, \
    .tran_list_size = sizeof(tran_list_##name) / sizeof(tran_list_##name[0]), \
    .entry_fn = entry, \
    .exit_fn = exit, \
}

void test_entry(void) { printf("%s\n", __func__); }
void test_exit(void) { printf("%s\n", __func__); }
void test_action(void) { printf("%s\n", __func__); }

STATE(test);
STATE(test2);

ASSIGN_TRAN(test, test_entry, test_exit, TRAN(2, test_action, test2), TRAN(4, NULL, test));
ASSIGN_TRAN(test2, NULL, NULL, TRAN(1, test_action, NULL), TRAN(3, NULL, test), TRAN(50, test_action, test2));


static const struct state *cur_state = &state_test;

void dispatch(int event) {
    printf("Dispatching event %d...\n", event);

    for (size_t i = 0; i < cur_state->tran_list_size; ++i) {
        if (cur_state->tran_list[i].event_n == event) {
            printf("Event %d found.\n", event);

            if (cur_state->exit_fn)
                cur_state->exit_fn();

            if(cur_state->tran_list[i].action_fn)
                cur_state->tran_list[i].action_fn();

            printf("Old state %p.\n", cur_state);
            cur_state = cur_state->tran_list[i].target_state;
            printf("New state %p.\n", cur_state);

            if(cur_state->entry_fn)
                cur_state->entry_fn();
            return;
        }
    }
    printf("Event %d not found in state %p\n", event, cur_state);
}

static void test() {
    printf("test:%p test2:%p NULL:%p\n", &state_test, &state_test2, &state_NULL);
    printf("test_entry:%p test_exit:%p\n", &test_entry, &test_exit);

    printf("Sen: %p, Sex: %p\n", state_test.entry_fn, state_test.exit_fn);
    printf("N: %ld\n", state_test.tran_list_size);
    for (int i = 0; i < state_test.tran_list_size; ++i) {
        printf("El: %d -- %p --> %p\n", state_test.tran_list[i].event_n, state_test.tran_list[i].action_fn, state_test.tran_list[i].target_state);
    }

    printf("Sen: %p, Sex: %p\n", state_test2.entry_fn, state_test2.exit_fn);
    printf("N: %ld\n", state_test2.tran_list_size);
    for (int i = 0; i < state_test2.tran_list_size; ++i) {
        printf("El: %d -- %p --> %p\n", state_test2.tran_list[i].event_n, state_test2.tran_list[i].action_fn, state_test2.tran_list[i].target_state);
    }

    dispatch(1);
    dispatch(2);
    dispatch(2);
    dispatch(4);
    dispatch(3);
}
