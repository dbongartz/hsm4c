#include <stdlib.h>
#include <stdio.h>

#include "hsm4c.h"

void test_entry(void *data) { printf("%s\n", __func__); }
void test_exit(void *data) { printf("%s\n", __func__); }
void test_action(void *data) { printf("%s\n", __func__); }
bool test_guard(void *data) { printf("%s\n", __func__); return true; }

DEFINE_STATE(test);
DEFINE_STATE(test2);
DEFINE_STATE(test3);

POPULATE_STATE(test, test_entry, test_exit,
    TRAN(2, test_action, test_guard, test2),
    TRAN(4, NULL, NULL, test)
);

POPULATE_STATE(test2, NULL, test_exit,
    TRAN(1, test_action, test_guard, test3),
    TRAN(3, NULL, test_guard, test),
    TRAN(50, test_action, NULL, test2)
);

POPULATE_STATE(test3, test_entry, NULL,
    TRAN(2, test_action, test_guard, test2)
);

static state_machine_t statem = { .state = &state_test, .data = NULL };

int main() {
    dispatch(&statem, 1);
    dispatch(&statem, 2);
    dispatch(&statem, 2);
    dispatch(&statem, 4);
    dispatch(&statem, 3);

    return EXIT_SUCCESS;
}