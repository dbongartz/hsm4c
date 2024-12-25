#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "hsm4c.h"
#include "trompeloeil/mock.hpp"
#include "trompeloeil/sequence.hpp"

using trompeloeil::_;

class StateFnMocks {
public:
  MAKE_MOCK1(entry_fn, void(hsm4c_state_t const *s));
  MAKE_MOCK2(run_fn, hsm4c_state_t *(hsm4c_state_t const *s, hsm4c_event_t e));
  MAKE_MOCK1(exit_fn, void(hsm4c_state_t const *s));
  MAKE_MOCK1(transition_fn, void(hsm4c_state_t const *root));
  MAKE_MOCK1(guard_fn, bool(hsm4c_state_t const *root));
  MAKE_MOCK1(choice_fn, bool(hsm4c_state_t const *root));
};

static StateFnMocks state_fn_mocks;

extern "C" {
void transition_fn(hsm4c_state_t const *root) { state_fn_mocks.transition_fn(root); }
bool guard_fn(hsm4c_state_t const *root) { return state_fn_mocks.guard_fn(root); }
void entry_fn(hsm4c_state_t const *s) { state_fn_mocks.entry_fn(s); }
hsm4c_state_t *run_fn(hsm4c_state_t const *s, hsm4c_event_t e) {
  return state_fn_mocks.run_fn(s, e);
}
void exit_fn(hsm4c_state_t const *s) { state_fn_mocks.exit_fn(s); }
bool choice_fn_A(hsm4c_state_t const *root) { return state_fn_mocks.choice_fn(root); }
bool choice_fn_B(hsm4c_state_t const *root) { return state_fn_mocks.choice_fn(root); }

enum states {
  ROOT,
  A,
  B,
  C,
  AA,
  AB,
  AC,
  BA,
  BB,
  BC,
  AAA,
  AAB,
  A_H,
  A_DH,
  A_CHOICE,
  B_H,
  _NUM_STATES
};

enum events { EV_1, EV_2, EV_3, EV_4, EV_5, EV_6, EV_7, EV_8, EV_9, EV_10, EV_11, EV_12 };

hsm4c_state_t states[_NUM_STATES] = {};

static hsm4c_transition_t const transitions_a[] = {
    {&states[A], &states[B], EV_1, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[A], &states[BB], EV_2, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[A], &states[B_H], EV_7, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_b[] = {
    {&states[B], &states[A], EV_1, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[B], &states[A_H], EV_3, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[B], &states[A_H], EV_4, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[B], &states[A_DH], EV_5, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aa[] = {
    {&states[AA], &states[AB], EV_3, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AA], &states[B], EV_4, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AA], &states[A_CHOICE], EV_6, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AA], &states[AAB], EV_9, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AA], &states[AAB], EV_10, transition_fn, guard_fn, HSM4C_TTYPE_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_ab[] = {
    {&states[AB], &states[B], EV_3, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aaa[] = {
    {&states[AAA], &states[AAB], EV_4, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AAA], &states[AAA], EV_8, transition_fn, guard_fn, HSM4C_TTYPE_EXTERNAL},
    {&states[AAA], &states[AAA], EV_11, transition_fn, guard_fn, HSM4C_TTYPE_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aab[] = {
    {&states[AAB], &states[AA], EV_12, transition_fn, guard_fn, HSM4C_TTYPE_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_a_choice[] = {
    {&states[A_CHOICE], &states[B], HSM4C_NO_EVENT, transition_fn, choice_fn_A,
     HSM4C_TTYPE_EXTERNAL},
    {&states[A_CHOICE], &states[C], HSM4C_NO_EVENT, transition_fn, choice_fn_B,
     HSM4C_TTYPE_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_state_config_t const statecfgs[_NUM_STATES] = {
    [ROOT] =
        {
            .name = "ROOT",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .initial = &states[A],
            .type = HSM4C_TYPE_ROOT,
            // .transitions = transitions_root, // Use for root hsm4c_transition_t testing
        },
    [A] =
        {
            .name = "A",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[ROOT],
            .initial = &states[AA],
            .transitions = transitions_a,
        },
    [B] =
        {
            .name = "B",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[ROOT],
            .initial = &states[BA],
            .transitions = transitions_b,
        },
    [C] =
        {
            .name = "C",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[ROOT],
        },
    [AA] =
        {
            .name = "AA",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[A],
            .initial = &states[AAA],
            .transitions = transitions_aa,
        },
    [AB] =
        {
            .name = "AB",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[A],
            .transitions = transitions_ab,
        },
    [AC] =
        {
            .name = "AC",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[A],
        },
    [BA] =
        {
            .name = "BA",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[B],
        },
    [BB] =
        {
            .name = "BB",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[B],
        },
    [BC] =
        {
            .name = "BC",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[B],
        },
    [AAA] =
        {
            .name = "AAA",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[AA],
            .transitions = transitions_aaa,
        },
    [AAB] =
        {
            .name = "AAB",
            .entry_fn = entry_fn,
            .run_fn = run_fn,
            .exit_fn = exit_fn,
            .parent = &states[AA],
            .transitions = transitions_aab,
        },
    [A_H] =
        {
            .name = "A_H",
            .parent = &states[A],
            .initial = &states[AB],
            .type = HSM4C_TYPE_HISTORY,
        },
    [B_H] =
        {
            .name = "B_H",
            .parent = &states[B],
            .initial = &states[BC],
            .type = HSM4C_TYPE_HISTORY,
        },
    [A_DH] =
        {
            .name = "A_HP",
            .parent = &states[A],
            .initial = &states[AC],
            .type = HSM4C_TYPE_HISTORY_DEEP,
        },
    [A_CHOICE] =
        {
            .name = "A_CHOICE",
            .parent = &states[A],
            .type = HSM4C_TYPE_CHOICE,
            .transitions = transitions_a_choice,
        },
};
} // extern "C"

/* -------- Tests -------- */

TEST_CASE("Initial state", "[trivial]") {
  trompeloeil::sequence seq;

  hsm4c_assign_stateconfigs_to_states(_NUM_STATES, states, statecfgs);

  REQUIRE_CALL(state_fn_mocks, entry_fn(&states[ROOT])).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, entry_fn(&states[A])).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, entry_fn(&states[AA])).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, entry_fn(&states[AAA])).IN_SEQUENCE(seq);

  hsm4c_init(&states[ROOT]);
}
