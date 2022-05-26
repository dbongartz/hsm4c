#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hsm4c.h"

void test_entry(void *data) { printf("Entry: %s\n", (char*)data); }
void test_exit(void *data) { printf("Exit: %s\n", (char*)data); }
void test_action(void *data) { printf("Action: %d\n", *(int*)data); }
bool test_guard(void *data) { printf("Guard ok: %d\n", *(int*)data); return true; }

DECLARE_STATE(root);
DECLARE_STATE(s1);
DECLARE_STATE(s2);
DECLARE_STATE(s3);
DECLARE_STATE(s4);
DECLARE_STATE(s5);

POPULATE_STATE(root, NULL, NULL, NULL, &state_s1);

POPULATE_STATE(s1, test_entry, test_exit, &state_root, &state_s2,
    TRAN(1, test_action, test_guard, s4),
    TRAN(3, test_action, test_guard, s3),
);

POPULATE_STATE(s2, test_entry, test_exit, &state_s1, NULL,
    TRAN(2, test_action, test_guard, s4),
);

POPULATE_STATE(s3, test_entry, test_exit, &state_root, &state_s5,
    TRAN(3, test_action, test_guard, s1)
);

POPULATE_STATE(s4, test_entry, test_exit, &state_s1, NULL,
    TRAN(2, test_action, test_guard, s2),
    TRAN(1, test_action, test_guard, s4)
);

POPULATE_STATE(s5, test_entry, test_exit, &state_s3, NULL,
    TRAN(2, test_action, test_guard, s2),
    TRAN(1, test_action, test_guard, s1)
);

int main() {
    dispatch_hsm(&state_root, 1);
    dispatch_hsm(&state_root, 2);
    dispatch_hsm(&state_root, 2);
    dispatch_hsm(&state_root, 1);
    dispatch_hsm(&state_root, 1);
    dispatch_hsm(&state_root, 3);
    dispatch_hsm(&state_root, 3);
    dispatch_hsm(&state_root, 2);

    return EXIT_SUCCESS;
}