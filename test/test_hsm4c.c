#include "unity.h"

#include <stdlib.h>
#include <stdio.h>

#include "../lib/hsm4c.h"

void s_entry(void *data) { printf("Entry: %s\n", (char *)data); }
void s_exit(void *data) { printf("Exit: %s\n", (char *)data); }
void t_action(void *data) { printf("Action: %d\n", *(int *)data); }
bool t_guard(void *data) {
  printf("Guard ok: %d\n", *(int *)data);
  return true;
}

DECLARE_STATE(root);
DECLARE_STATE(s1);
DECLARE_STATE(s2);
DECLARE_STATE(s3);
DECLARE_STATE(s4);
DECLARE_STATE(s5);

POPULATE_STATE(root, NULL, NULL, NONE, s1, STATE_FLAG_NONE);

POPULATE_STATE(s1, s_entry, s_exit, root, s2, STATE_FLAG_NONE,
               TRAN(1, t_action, t_guard, s4),
               TRAN(3, t_action, t_guard, s3), );

POPULATE_STATE(s2, s_entry, s_exit, s1, NONE, STATE_FLAG_NONE,
               TRAN(2, t_action, t_guard, s4), );

POPULATE_STATE(s3, s_entry, s_exit, root, s5, STATE_FLAG_NONE,
               TRAN(3, t_action, t_guard, s1));

POPULATE_STATE(s4, s_entry, s_exit, s1, NONE, STATE_FLAG_NONE,
               TRAN(2, t_action, t_guard, s2),
               TRAN(1, t_action, t_guard, s4));

POPULATE_STATE(s5, s_entry, s_exit, s3, NONE, STATE_FLAG_NONE,
               TRAN(2, t_action, t_guard, s2),
               TRAN(1, t_action, t_guard, s1));

void setUp(void) {}

void tearDown(void) {}

void test_S1E1ExpectS4(void) {
  dispatch_hsm(STATE_REF(root), 1);
  TEST_ASSERT_EQUAL_STRING(STATE_DEF(s4)._state->name, get_state(STATE_REF(root))->_state->name);
}
