#include <stdlib.h>
#include <stdio.h>

#include "hsm4c.h"

void test_entry(void *data) { printf("%s\n", __func__); }
void test_exit(void *data) { printf("%s\n", __func__); }
void test_action(void *data) { printf("%s\n", __func__); }
bool test_guard(void *data) { printf("%s\n", __func__); return true; }
bool test_guard_false(void *data) { printf("%s\n", __func__); return false; }

DECLARE_STATE(s1);
DECLARE_STATE(s2);
DECLARE_STATE(s3);

POPULATE_STATE(s1, test_entry, test_exit,
    TRAN(1, test_action, test_guard, &s2),
    TRAN(3, test_action, test_guard_false, &s1),
    TRAN(2, test_action, test_guard, &s1),
    TRAN(3, test_action, test_guard, &s3),
);

POPULATE_STATE(s2, test_entry, test_exit,
    TRAN(1, test_action, test_guard, &s3),
    TRAN(2, test_action, test_guard, &s2),
    TRAN(3, test_action, test_guard, &s1)
);

POPULATE_STATE(s3, NULL, NULL,
    TRAN(1, NULL, NULL, &s1),
    TRAN(2, test_action, NULL, &s3),
    TRAN(3, NULL, NULL, &s2)
);

DEFINE_STATEM(machine, s1, NULL);

int main() {

    while (1) {
        printf("Event: ");

        int event;
        scanf("%d", &event);
        dispatch(&machine, event);
    }

    return EXIT_SUCCESS;
}