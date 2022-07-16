#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/hsm4c.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

enum action_type {
  ACTION_NULL,
  ACTION_ENTRY,
  ACTION_EXIT,
  ACTION_TRANSITION,
  ACTION_GUARD_OK,
  ACTION_GUARD_FAIL,
};

struct action {
  enum action_type type;
  char *data;
};

static struct action action_order_buffer[30] = {};
static size_t action_order_nr = 0;

void s_entry(void *data) {
  fflush(stdout);
  action_order_buffer[action_order_nr].type = ACTION_ENTRY;
  action_order_nr++;
}
void s_exit(void *data) {
  action_order_buffer[action_order_nr].type = ACTION_EXIT;
  action_order_nr++;
}
void t_action(void *data) {
  action_order_buffer[action_order_nr].type = ACTION_TRANSITION;
  action_order_nr++;
}
bool t_guard_ok(void *e) {
  action_order_buffer[action_order_nr].type = ACTION_GUARD_OK;
  action_order_nr++;
  return true;
}

bool t_guard_fail(void *e) {
  action_order_buffer[action_order_nr].type = ACTION_GUARD_FAIL;
  action_order_nr++;
  return false;
}

enum events {
  EV_NEXT,
  EV_PREV,
  EV_SELF_INTERNAL,
  EV_SELF_EXTERNAL,
  EV_GUARD_NEXT,
  EV_NOT_USED
};

DECLARE_STATE(s1);
DECLARE_STATE(s2);
DECLARE_STATE(s3);

POPULATE_STATE(s1, s_entry, s_exit,
               TRAN(EV_NEXT, t_action, t_guard_ok, &s2),
               TRAN(EV_PREV, t_action, t_guard_ok, &s3),
               TRAN_INTERNAL(EV_SELF_INTERNAL, t_action, t_guard_ok),
               TRAN(EV_SELF_EXTERNAL, t_action, t_guard_ok, &s1),
               TRAN(EV_NEXT, t_action, t_guard_ok,
                    &s1), // Should never be reached
               TRAN(EV_GUARD_NEXT, t_action, t_guard_fail,
                    &s1), // Should never be executed
               TRAN(EV_GUARD_NEXT, t_action, t_guard_ok, &s2), );

POPULATE_STATE(s2, s_entry, s_exit,
               TRAN(EV_NEXT, t_action, t_guard_ok, &s3),
               TRAN(EV_PREV, t_action, t_guard_ok, &s1),
               TRAN_INTERNAL(EV_SELF_INTERNAL, t_action, t_guard_ok),
               TRAN(EV_SELF_EXTERNAL, t_action, t_guard_ok, &s2),
               TRAN(EV_NEXT, t_action, t_guard_ok,
                    &s2), // Should never be reached
               TRAN(EV_GUARD_NEXT, t_action, t_guard_fail,
                    &s2), // Should never be executed
               TRAN(EV_GUARD_NEXT, t_action, t_guard_ok, &s3), );

POPULATE_STATE(s3, s_entry, s_exit,
               TRAN(EV_NEXT, t_action, t_guard_ok, &s1),
               TRAN(EV_PREV, t_action, t_guard_ok, &s2),
               TRAN_INTERNAL(EV_SELF_INTERNAL, t_action, t_guard_ok),
               TRAN(EV_SELF_EXTERNAL, t_action, t_guard_ok, &s3),
               TRAN(EV_NEXT, t_action, t_guard_ok,
                    &s3), // Should never be reached
               TRAN(EV_GUARD_NEXT, t_action, t_guard_fail,
                    &s3), // Should never be executed
               TRAN(EV_GUARD_NEXT, t_action, t_guard_ok, &s1), );

DEFINE_STATEM(machine, s1, NULL);

void test_s1_not_used_should_be_ignored(void) {
  struct action action_order_expected[] = {{ACTION_NULL}};

  dispatch(&machine, EV_NOT_USED);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s1).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

void test_s1_next_should_be_s2(void) {
  struct action action_order_expected[] = {{ACTION_GUARD_OK},
                                           {ACTION_EXIT},
                                           {ACTION_TRANSITION},
                                           {ACTION_ENTRY},
                                           {ACTION_NULL}};

  dispatch(&machine, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s2).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

void test_s1_self_internal_should_be_s1_no_exit_entry(void) {
  struct action action_order_expected[] = {{ACTION_GUARD_OK},
                                           {ACTION_TRANSITION},
                                           {ACTION_NULL}};

  dispatch(&machine, EV_SELF_INTERNAL);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s1).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

void test_s1_self_external_should_be_s1_with_exit_entry(void) {
  struct action action_order_expected[] = {{ACTION_GUARD_OK},
                                           {ACTION_EXIT},
                                           {ACTION_TRANSITION},
                                           {ACTION_ENTRY},
                                           {ACTION_NULL}};

  dispatch(&machine, EV_SELF_EXTERNAL);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s1).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

void test_s1_next_next_should_be_s3(void) {
  struct action action_order_expected_1[] = {{ACTION_GUARD_OK},
                                             {ACTION_EXIT},
                                             {ACTION_TRANSITION},
                                             {ACTION_ENTRY},
                                             {ACTION_NULL}};

  dispatch(&machine, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s2).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected_1, action_order_buffer,
                               ARRAY_LEN(action_order_expected_1));

  struct action action_order_expected_2[] = {
      {ACTION_GUARD_OK},   {ACTION_EXIT},     {ACTION_TRANSITION},
      {ACTION_ENTRY},      {ACTION_GUARD_OK}, {ACTION_EXIT},
      {ACTION_TRANSITION}, {ACTION_ENTRY},    {ACTION_NULL}};

  dispatch(&machine, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s3).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected_2, action_order_buffer,
                               ARRAY_LEN(action_order_expected_2));
}

void test_s1_guard_next_should_be_s1(void) {
  struct action action_order_expected[] = {{ACTION_GUARD_FAIL},
                                           {ACTION_GUARD_OK},
                                           {ACTION_EXIT},
                                           {ACTION_TRANSITION},
                                           {ACTION_ENTRY},
                                           {ACTION_NULL}};

  dispatch(&machine, EV_GUARD_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s2).name, machine.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

/* --------- NULL tests --------- */

DECLARE_STATE(s1_null);
DECLARE_STATE(s2_null);
DECLARE_STATE(s3_null);

POPULATE_STATE(s1_null, NULL, NULL,
               TRAN(EV_NEXT, NULL, NULL, &s2_null),
               TRAN(EV_PREV, NULL, NULL, &s3_null),
               TRAN_INTERNAL(EV_SELF_INTERNAL, NULL, NULL),
               TRAN(EV_SELF_EXTERNAL, NULL, NULL, &s1_null), );

POPULATE_STATE(s2_null, NULL, NULL,
               TRAN(EV_NEXT, NULL, NULL, &s3_null),
               TRAN(EV_PREV, NULL, NULL, &s1_null),
               TRAN_INTERNAL(EV_SELF_INTERNAL, NULL, NULL),
               TRAN(EV_SELF_EXTERNAL, NULL, NULL, &s2_null), );

POPULATE_STATE(s3_null, NULL, NULL,
               TRAN(EV_NEXT, NULL, NULL, &s1_null),
               TRAN(EV_PREV, NULL, NULL, &s2_null),
               TRAN_INTERNAL(EV_SELF_INTERNAL, NULL, NULL),
               TRAN(EV_SELF_EXTERNAL, NULL, NULL, &s3_null), );

DEFINE_STATEM(machine_null, s1_null, NULL);

void test_s1_null_3x_next_should_be_s1_null_with_no_actions(void) {
  struct action action_order_expected[] = {
      {ACTION_NULL},
  };

  dispatch(&machine_null, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s2_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));

  dispatch(&machine_null, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s3_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));

  dispatch(&machine_null, EV_NEXT);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s1_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}

void test_s1_null_3x_prev_should_be_s1_null_with_no_actions(void) {
  struct action action_order_expected[] = {
      {ACTION_NULL},
  };

  dispatch(&machine_null, EV_PREV);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s3_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));

  dispatch(&machine_null, EV_PREV);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s2_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));

  dispatch(&machine_null, EV_PREV);
  TEST_ASSERT_EQUAL_STRING(STATE_NAME(s1_null).name, machine_null.state->name);
  TEST_ASSERT_EQUAL_INT8_ARRAY(action_order_expected, action_order_buffer,
                               ARRAY_LEN(action_order_expected));
}


void setUp(void) {
  memset(action_order_buffer, 0, sizeof(action_order_buffer));
  action_order_nr = 0;
  machine.state = &s1;
  machine_null.state = &s1_null;
}

void tearDown(void) {}