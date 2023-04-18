#include "support_fns.h"
#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/hsm4c.h"

#include "mock_support_fns.h"
// #include "support_fns.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

/* -------- TEST FIXTURE -------- */

static bool t_choice_A_return = false;
static int t_choice_A_called = 0;
static bool t_choice_B_return = false;
static int t_choice_B_called = 0;

static void reset_choice_A(void) {
  t_choice_A_return = false;
  t_choice_A_called = 0;
}

static void reset_choice_B(void) {
  t_choice_B_return = false;
  t_choice_B_called = 0;
}

void reset_all_states(size_t num_states, State states[num_states]) {
  for (size_t i = 0; i < num_states; ++i) {
    sc_reset_state(&states[i]);
  }
}

static bool t_choice_A(State const *s) {
  t_choice_A_called++;
  return t_choice_A_return;
}

static bool t_choice_B(State const *s) {
  t_choice_B_called++;
  return t_choice_B_return;
}

static void ignore_state_and_transition_fn(void) {
  s_entry_Ignore();
  s_exit_Ignore();
  s_run_IgnoreAndReturn(NULL);
  t_action_Ignore();
  t_guard_IgnoreAndReturn(true);
}

static void stop_ignore_state_and_transition_fn(void) {
  s_entry_StopIgnore();
  s_exit_StopIgnore();
  s_run_StopIgnore();
  t_action_StopIgnore();
  t_guard_StopIgnore();
}

enum states { ROOT, A, B, C, AA, AB, AC, BA, BB, BC, AAA, AAB, A_H, A_DH, A_CHOICE, B_H };
State states[] = {
    [ROOT] =
        {
            .name = "ROOT",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .initial = &states[A],
            .type = SC_TYPE_ROOT,
        },
    [A] =
        {
            .name = "A",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
            .initial = &states[AA],
        },
    [B] =
        {
            .name = "B",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
            .initial = &states[BA],
        },
    [C] =
        {
            .name = "C",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
        },
    [AA] =
        {
            .name = "AA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
            .initial = &states[AAA],
        },
    [AB] =
        {
            .name = "AB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
        },
    [AC] =
        {
            .name = "AC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
        },
    [BA] =
        {
            .name = "BA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [BB] =
        {
            .name = "BB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [BC] =
        {
            .name = "BC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [AAA] =
        {
            .name = "AAA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[AA],
        },
    [AAB] =
        {
            .name = "AAB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[AA],
        },
    [A_H] =
        {
            .name = "A_H",
            .parent = &states[A],
            .initial = &states[AB],
            .type = SC_TYPE_HISTORY,
        },
    [B_H] =
        {
            .name = "B_H",
            .parent = &states[B],
            .initial = &states[BC],
            .type = SC_TYPE_HISTORY,
        },
    [A_DH] =
        {
            .name = "A_HP",
            .parent = &states[A],
            .initial = &states[AC],
            .type = SC_TYPE_HISTORY_DEEP,
        },
    [A_CHOICE] =
        {
            .name = "A_CHOICE",
            .parent = &states[A],
            .type = SC_TYPE_CHOICE,
        },
};

enum events {
  EV_NO_EVENT = SC_NO_EVENT,
  EV_1,
  EV_2,
  EV_3,
  EV_4,
  EV_5,
  EV_6,
  EV_7,
  EV_8,
  EV_9,
  EV_10,
  EV_11,
  EV_12
};
static Transition const transitions[] = {
    {&states[A], &states[B], EV_1, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[B], &states[A], EV_1, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[A], &states[BB], EV_2, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AA], &states[AB], EV_3, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AB], &states[B], EV_3, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[B], &states[A_H], EV_3, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AAA], &states[AAB], EV_4, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AA], &states[B], EV_4, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[B], &states[A_H], EV_4, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[B], &states[A_DH], EV_5, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AA], &states[A_CHOICE], EV_6, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[A_CHOICE], &states[B], EV_NO_EVENT, t_action, t_choice_A, SC_TTYPE_EXTERNAL},
    {&states[A_CHOICE], &states[C], EV_NO_EVENT, t_action, t_choice_B, SC_TTYPE_EXTERNAL},
    {&states[A], &states[B_H], EV_7, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AAA], &states[AAA], EV_8, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AA], &states[AAB], EV_9, t_action, t_guard, SC_TTYPE_EXTERNAL},
    {&states[AA], &states[AAB], EV_10, t_action, t_guard, SC_TTYPE_INTERNAL},
    {&states[AAB], &states[AA], EV_12, t_action, t_guard, SC_TTYPE_INTERNAL},
    {&states[AAA], &states[AAA], EV_11, t_action, t_guard, SC_TTYPE_INTERNAL},
    SC_TRANSITIONS_END,
};

void setUp(void) {
  reset_choice_A();
  reset_choice_B();
  reset_all_states(ARRAY_LEN(states), states);
}

void tearDown(void) {}

/* -------- TESTS -------- */

void test_initial(void) {
  s_entry_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAA]);

  sc_init(&states[ROOT], transitions);

  // Double init to check if it resets accordingly
  s_entry_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAA]);

  sc_init(&states[ROOT], transitions);
}

void test_sc_A_to_B(void) {
  s_entry_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAA]);

  sc_init(&states[ROOT], transitions);

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[B]);
  s_entry_Expect(&states[BA]);
  s_run_ExpectAndReturn(&states[BA], EV_1, NULL);
  s_run_ExpectAndReturn(&states[B], EV_1, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_1, NULL);

  sc_run(&states[ROOT], EV_1);
}

void test_sc_B_to_A(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_1);

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[BA]);
  s_exit_Expect(&states[B]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAA]);
  s_run_ExpectAndReturn(&states[AAA], EV_1, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_1, NULL);
  s_run_ExpectAndReturn(&states[A], EV_1, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_1, NULL);

  sc_run(&states[ROOT], EV_1);
}

void test_sc_A_to_BB(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_1);
  sc_run(&states[ROOT], EV_1);

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[B]);
  s_entry_Expect(&states[BB]);
  s_run_ExpectAndReturn(&states[BB], EV_2, NULL);
  s_run_ExpectAndReturn(&states[B], EV_2, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_2, NULL);

  sc_run(&states[ROOT], EV_2);
}

void test_sc_AA_to_AB(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AB]);
  s_run_ExpectAndReturn(&states[AB], EV_3, NULL);
  s_run_ExpectAndReturn(&states[A], EV_3, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_3, NULL);

  sc_run(&states[ROOT], EV_3);
}

void test_sc_AA_to_AB_to_B_to_A_History(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_3);
  stop_ignore_state_and_transition_fn();

  // Now in A->AB

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AB]);
  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[B]);
  s_entry_Expect(&states[BA]);
  s_run_ExpectAndReturn(&states[BA], EV_3, NULL);
  s_run_ExpectAndReturn(&states[B], EV_3, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_3, NULL);

  sc_run(&states[ROOT], EV_3);

  // Now in B with A->AB History. We expect to land back in AB

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[BA]);
  s_exit_Expect(&states[B]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AB]);
  s_run_ExpectAndReturn(&states[AB], EV_3, NULL);
  s_run_ExpectAndReturn(&states[A], EV_3, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_3, NULL);

  sc_run(&states[ROOT], EV_3);
}

void test_sc_AAA_to_AAB(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AAB]);
  s_run_ExpectAndReturn(&states[AAB], EV_4, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_4, NULL);
  s_run_ExpectAndReturn(&states[A], EV_4, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_4, NULL);

  sc_run(&states[ROOT], EV_4);
}

void test_sc_AAA_to_AAB_to_B_to_A_History(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_4);
  // Now in AAB
  sc_run(&states[ROOT], EV_4);
  // Now in B with A->AA->AAB History. Expecting AAA

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[BA]);
  s_exit_Expect(&states[B]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAA]);
  s_run_ExpectAndReturn(&states[AAA], EV_4, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_4, NULL);
  s_run_ExpectAndReturn(&states[A], EV_4, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_4, NULL);

  sc_run(&states[ROOT], EV_4);
}

void test_sc_AAA_to_AAB_to_B_to_A_DeepHistory(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_4);
  // Now in AAB
  sc_run(&states[ROOT], EV_4);
  // Now in B with A->AA->AAB Deep History. Expecting AAA.

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[BA]);
  s_exit_Expect(&states[B]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[A]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAB]);
  s_run_ExpectAndReturn(&states[AAB], EV_5, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_5, NULL);
  s_run_ExpectAndReturn(&states[A], EV_5, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_5, NULL);

  sc_run(&states[ROOT], EV_5);
}

void test_sc_A_to_C_choice(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  t_action_Expect(&states[ROOT]);
  s_run_ExpectAndReturn(&states[A], EV_6, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_6, NULL);

  sc_run(&states[ROOT], EV_6);

  TEST_ASSERT_EQUAL_INT(1, t_choice_A_called);
  TEST_ASSERT_EQUAL_INT(1, t_choice_B_called);

  t_choice_B_return = true;
  // Should go to C next poll with a non matching or SC_NO_EVENT

  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[C]);
  s_run_ExpectAndReturn(&states[C], EV_6, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_6, NULL);

  sc_run(&states[ROOT], EV_6);
}

void test_sc_A_to_B_choice_auto(void) {
  ignore_state_and_transition_fn();

  sc_init(&states[ROOT], transitions);

  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  t_action_Expect(&states[ROOT]); // Action of AA->A_CHOICE
  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]); // Action of A_CHOICE->B
  s_entry_Expect(&states[B]);
  s_entry_Expect(&states[BA]);
  s_run_ExpectAndReturn(&states[BA], EV_6, NULL);
  s_run_ExpectAndReturn(&states[B], EV_6, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_6, NULL);
  t_choice_A_return = true;

  // Should go to B immediately

  sc_run(&states[ROOT], EV_6);

  TEST_ASSERT_EQUAL_INT(1, t_choice_A_called);
  TEST_ASSERT_EQUAL_INT(0, t_choice_B_called);
}

void test_sc_A_to_B_History_with_no_history_set(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  s_exit_Expect(&states[A]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[B]);
  s_entry_Expect(&states[BC]);
  s_run_ExpectAndReturn(&states[BC], EV_7, NULL);
  s_run_ExpectAndReturn(&states[B], EV_7, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_7, NULL);

  sc_run(&states[ROOT], EV_7);
}

void test_sc_AAA_to_AAA_external(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AAA]);
  s_run_ExpectAndReturn(&states[AAA], EV_8, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_8, NULL);
  s_run_ExpectAndReturn(&states[A], EV_8, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_8, NULL);

  sc_run(&states[ROOT], EV_8);
}

void test_sc_AA_to_AAB_external(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  s_exit_Expect(&states[AA]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AA]);
  s_entry_Expect(&states[AAB]);
  s_run_ExpectAndReturn(&states[AAB], EV_9, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_9, NULL);
  s_run_ExpectAndReturn(&states[A], EV_9, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_9, NULL);

  sc_run(&states[ROOT], EV_9);
}

void test_sc_AA_to_AAB_internal(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAA]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AAB]);
  s_run_ExpectAndReturn(&states[AAB], EV_10, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_10, NULL);
  s_run_ExpectAndReturn(&states[A], EV_10, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_10, NULL);

  sc_run(&states[ROOT], EV_10);
}

void test_sc_AAB_to_AA_internal(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  sc_run(&states[ROOT], EV_10);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  s_exit_Expect(&states[AAB]);
  t_action_Expect(&states[ROOT]);
  s_entry_Expect(&states[AAB]); // Why is this? How should it be?
  s_run_ExpectAndReturn(&states[AAB], EV_10, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_10, NULL);
  s_run_ExpectAndReturn(&states[A], EV_10, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_10, NULL);

  sc_run(&states[ROOT], EV_10);
  TEST_MESSAGE("TODO. Don't undertand the specs yet");
}

void test_sc_AAA_to_AAA_internal(void) {
  ignore_state_and_transition_fn();
  sc_init(&states[ROOT], transitions);
  stop_ignore_state_and_transition_fn();

  t_guard_ExpectAndReturn(&states[ROOT], true);
  t_action_Expect(&states[ROOT]);
  s_run_ExpectAndReturn(&states[AAA], EV_11, NULL);
  s_run_ExpectAndReturn(&states[AA], EV_11, NULL);
  s_run_ExpectAndReturn(&states[A], EV_11, NULL);
  s_run_ExpectAndReturn(&states[ROOT], EV_11, NULL);

  sc_run(&states[ROOT], EV_11);
}

/*
 * TODO:
 *
 * - Empty state and transition functions
 * - No transition found
 * - false guard
 * - Parent state with no initial (Should work as target and with child as target)
 * - Auto transition on normal states
 * - Auto transitions on history states (Should not execute!)
 * - initial != self (can be cought)
 * - initial != other parent (can be cought)
 * - State change via run functions on lowest and high level
 * - State change via run on no transition found
 * - State change via run automatic
 */
