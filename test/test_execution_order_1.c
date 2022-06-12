#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/hsm4c.h"

enum action_type {
	ACTION_NULL,
	ACTION_ENTRY,
	ACTION_EXIT,
	ACTION_TRANSITION,
	ACTION_GUARD,
	ACTION_INITIAL,
};

struct action {
	enum action_type type;
	char *data;
};
static struct action action_order_buffer[30];
static size_t action_order_nr = 0;

void s_entry(void *data) {
	printf("Entry: %s\n", ((state_t *)data)->_state->name);
	fflush(stdout);
	action_order_buffer[action_order_nr].type = ACTION_ENTRY;
	action_order_buffer[action_order_nr].data = malloc(21);
	strncpy(action_order_buffer[action_order_nr].data, ((state_t *)data)->_state->name, 20);
	action_order_nr++;
}
void s_exit(void *data) {
	printf("Exit: %s\n", ((state_t *)data)->_state->name); fflush(stdout);
	action_order_buffer[action_order_nr].type = ACTION_EXIT;
	action_order_buffer[action_order_nr].data = malloc(21);
	strncpy(action_order_buffer[action_order_nr].data, ((state_t *)data)->_state->name, 20);
	action_order_nr++;
}
void s_initial(void *data) {
	printf("Initial: %s\n", ((state_t *)data)->_state->name); fflush(stdout);
	action_order_buffer[action_order_nr].type = ACTION_INITIAL;
	action_order_buffer[action_order_nr].data = malloc(21);
	strncpy(action_order_buffer[action_order_nr].data, ((state_t *)data)->_state->name, 20);
	action_order_nr++;
}
void t_action(event_t e) {
	printf("Transition: %d\n", e.num); fflush(stdout);
	action_order_buffer[action_order_nr].type = ACTION_TRANSITION;
	action_order_buffer[action_order_nr].data = malloc(21);
	sprintf(action_order_buffer[action_order_nr].data, "%d", e.num);
	action_order_nr++;
}
bool t_guard(event_t e) {
	printf("Guard ok: %d\n", e.num); fflush(stdout);
	action_order_buffer[action_order_nr].type = ACTION_GUARD;
	action_order_buffer[action_order_nr].data = malloc(21);
	sprintf(action_order_buffer[action_order_nr].data, "%d", e.num);
	action_order_nr++;
	return true;
}

DECLARE_STATE(root);
DECLARE_STATE(s)
DECLARE_STATE(s1);
DECLARE_STATE(s2);
DECLARE_STATE(s11);
DECLARE_STATE(s12);
DECLARE_STATE(s21);

POPULATE_STATE(root, NULL, NULL, NONE, s, s_initial, 0);

POPULATE_STATE(s, s_entry, s_exit, root, s1, s_initial, 0);
POPULATE_STATE(s1, s_entry, s_exit, s, s11, s_initial, STATE_FLAG_HISTORY, TRAN(12, t_action, t_guard, s2), TRAN(11, t_action, t_guard, STATE_SELF));
POPULATE_STATE(s2, s_entry, s_exit, s, s21, s_initial, 0, TRAN(21, t_action, t_guard, s1));
POPULATE_STATE(s11, s_entry, s_exit, s1, NONE, s_initial, 0, TRAN(1121, t_action, t_guard, s21), TRAN(1112, t_action, t_guard, s12));
POPULATE_STATE(s12, s_entry, s_exit, s1, NONE, s_initial, 0, TRAN(1212, t_action, t_guard, STATE_SELF));
POPULATE_STATE(s21, s_entry, s_exit, s2, NONE, s_initial, 0, TRAN(2121, t_action, t_guard, s21));

void setUp(void) {
	memset(action_order_buffer, 0, sizeof(action_order_buffer));
	action_order_nr = 0;
}

void tearDown(void) {
	for (size_t i = 0; i < 30 && action_order_buffer[i].type != ACTION_NULL;
			 i++) {
		free(action_order_buffer[i].data);
	}
	reset_statemachine(get_statemachine(STATE_REF(root)));
}

void test_e1(void) {
	struct action action_order_buffer_expected[30] = {
		{ACTION_GUARD, "12"},
		{ACTION_EXIT, "s11"},
		{ACTION_EXIT, "s1"},
		{ACTION_TRANSITION, "12"},
		{ACTION_ENTRY, "s2"},
		{ACTION_INITIAL, "s21"},
		{ACTION_ENTRY, "s21"},
	};

	dispatch_hsm(STATE_REF(root), 12, NULL);
	TEST_ASSERT_EQUAL_STRING(STATE_DEF(s21)._state->name,
													get_state(&state_root)->_state->name);

	for(size_t i = 0; i < 30 && action_order_buffer[i].type != ACTION_NULL; i++) {
		TEST_ASSERT_EQUAL_INT(action_order_buffer_expected[i].type, action_order_buffer[i].type);
		TEST_ASSERT_EQUAL_STRING(action_order_buffer_expected[i].data, action_order_buffer[i].data);
	}
}

void test_external_self_trans(void) {
  struct action action_order_buffer_expected[30] = {
      {ACTION_GUARD, "12"},        {ACTION_EXIT, "s11"},
      {ACTION_EXIT, "s1"}, {ACTION_TRANSITION, "12"},
      {ACTION_ENTRY, "s2"}, {ACTION_INITIAL, "s21"}, {ACTION_ENTRY, "s21"},
      {ACTION_GUARD, "2121"},      {ACTION_EXIT, "s21"},
      {ACTION_TRANSITION, "2121"}, {ACTION_ENTRY, "s21"},
  };

  dispatch_hsm(STATE_REF(root), 12, NULL);
  dispatch_hsm(STATE_REF(root), 2121, NULL);
  TEST_ASSERT_EQUAL_STRING(STATE_DEF(s21)._state->name,
                           get_state(&state_root)->_state->name);

  for (size_t i = 0; i < 30; i++) {
    TEST_ASSERT_EQUAL_INT(action_order_buffer_expected[i].type,
                          action_order_buffer[i].type);
    TEST_ASSERT_EQUAL_STRING(action_order_buffer_expected[i].data,
                             action_order_buffer[i].data);
	}
}

void test_internal_self_trans(void) {
	struct action action_order_buffer_expected[30] = {
			{ACTION_GUARD, "11"}, {ACTION_TRANSITION, "11"}
	};

	dispatch_hsm(STATE_REF(root), 11, NULL);
	TEST_ASSERT_EQUAL_STRING(STATE_DEF(s11)._state->name,
													 get_state(&state_root)->_state->name);

	for (size_t i = 0; i < 30; i++) {
		TEST_ASSERT_EQUAL_INT_MESSAGE(action_order_buffer_expected[i].type,
													action_order_buffer[i].type, "TYPE");
		TEST_ASSERT_EQUAL_STRING_MESSAGE(action_order_buffer_expected[i].data,
														 action_order_buffer[i].data, "DATA");
	}
}

void test_history(void) {
	struct action action_order_buffer_expected[30] = {
			{ACTION_GUARD, "1112"}, {ACTION_EXIT, "s11"},
			{ACTION_TRANSITION, "1112"}, {ACTION_ENTRY, "s12"},
			{ACTION_GUARD, "12"}, {ACTION_EXIT, "s12"}, {ACTION_EXIT, "s1"},
			{ACTION_TRANSITION, "12"}, {ACTION_ENTRY, "s2"},  {ACTION_INITIAL, "s21"}, {ACTION_ENTRY, "s21"},
			{ACTION_GUARD, "21"}, {ACTION_EXIT, "s21"}, {ACTION_EXIT, "s2"},
			{ACTION_TRANSITION, "21"}, {ACTION_ENTRY, "s1"},  {ACTION_ENTRY, "s12"},
	};

	dispatch_hsm(STATE_REF(root), 1112, NULL);
	TEST_ASSERT_EQUAL_STRING(STATE_DEF(s12)._state->name, get_state(&state_root)->_state->name);
	dispatch_hsm(STATE_REF(root), 12, NULL);
	TEST_ASSERT_EQUAL_STRING(STATE_DEF(s21)._state->name, get_state(&state_root)->_state->name);
	dispatch_hsm(STATE_REF(root), 21, NULL);
	TEST_ASSERT_EQUAL_STRING(STATE_DEF(s12)._state->name, get_state(&state_root)->_state->name);

	for (size_t i = 0; i < 30; i++) {
		TEST_ASSERT_EQUAL_INT_MESSAGE(action_order_buffer_expected[i].type,
													action_order_buffer[i].type, "TYPE" );
		TEST_ASSERT_EQUAL_STRING_MESSAGE(action_order_buffer_expected[i].data,
														 action_order_buffer[i].data, "DATA");
	}
}

