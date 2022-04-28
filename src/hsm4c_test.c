#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "hsm4c.h"

void test_entry(void *data) { printf("Entry: %s\n", (char*)data); }
void test_exit(void *data) { printf("Exit: %s\n", (char*)data); }
void test_action(void *data) { printf("Action: %d\n", *(int*)data); }
bool test_guard(void *data) { printf("Guard ok: %d\n", *(int*)data); return true; }

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

static state_m_t statem = { .state = &state_test, .data = NULL };


DEFINE_STATE(root);
DEFINE_STATE(s1);
DEFINE_STATE(s2);
DEFINE_STATE(s3);
DEFINE_STATE(s4);

POPULATE_STATE(s1, test_entry, test_exit,
    TRAN(1, test_action, test_guard, s3),
);

POPULATE_STATE(s2, test_entry, test_exit,
    TRAN(2, test_action, test_guard, s4),
);

POPULATE_STATE(s3, test_entry, test_exit,
    TRAN(1, test_action, test_guard, s1)
);

POPULATE_STATE(s4, test_entry, test_exit,
    TRAN(2, test_action, test_guard, s2),
    TRAN(1, test_action, test_guard, s4)
);

int main() {
    // dispatch(&statem, 1);
    // dispatch(&statem, 2);
    // dispatch(&statem, 2);
    // dispatch(&statem, 4);
    // dispatch(&statem, 3);

    state_root.parent = NULL;
    state_root.child = &state_s1;
    state_root.initial = &state_s1;

    state_s1.parent = &state_root;
    state_s1.child = &state_s2;
    state_s1.initial = &state_s2;

    state_s2.parent = &state_s1;
    state_s2.child = NULL;
    state_s2.initial = NULL;

    state_s3.parent = &state_root;
    state_s3.child = &state_s4;
    state_s3.initial = &state_s4;

    state_s4.parent = &state_s3;
    state_s4.child = NULL;
    state_s4.initial = NULL;

    dispatch_hsm(&state_root, 1);
    assert(state_root.child == &state_s3);
    assert(state_s3.child == &state_s4);

    dispatch_hsm(&state_root, 2);
    assert(state_root.child == &state_s1);
    assert(state_s1.child == &state_s2);

    dispatch_hsm(&state_root, 2);
    assert(state_root.child == &state_s3);
    assert(state_s3.child == &state_s4);

    dispatch_hsm(&state_root, 1);





    return EXIT_SUCCESS;
}