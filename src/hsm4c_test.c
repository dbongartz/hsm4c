#include <stdlib.h>
#include <stdio.h>

#include "hsm4c.h"

void test_entry(void *data) { printf("%s\n", __func__); }
void test_exit(void *data) { printf("%s\n", __func__); }
void test_action(void *data) { printf("%s\n", __func__); }
bool test_guard(void *data) { printf("%s\n", __func__); return true; }

STATE(test);
STATE(test2);

ASSIGN_TRAN(test, test_entry, test_exit, TRAN(2, test_action, test_guard, test2), TRAN(4, NULL, NULL, test));
ASSIGN_TRAN(test2, NULL, NULL, TRAN(1, test_action, test_guard, test), TRAN(3, NULL, test_guard, test), TRAN(50, test_action, NULL, test2));

static state_m_t my_statem = { .state = &state_test, .data = NULL };

int main() {
    printf("Hello, World!\n");

    test(&my_statem);

    return EXIT_SUCCESS;
}