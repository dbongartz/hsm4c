#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>
#include <cstdint>

#include "hsm4c.h"

namespace Fakes {
#include "trompeloeil/mock.hpp"
class StateFnFakes {
 public:
  // NOLINTBEGIN
  MAKE_MOCK2(entry_fn, void(void *ctx, hsm4c_state_t const *state));
  MAKE_MOCK2(exit_fn, void(void *ctx, hsm4c_state_t const *state));

  // MAKE_MOCK1(transition_fn, void(hsm4c_event_t e));
  // MAKE_MOCK1(guard_fn, bool(hsm4c_event_t e));
  // MAKE_MOCK1(choice_fn, hsm4c_target_state_t(hsm4c_event_t e));
};

namespace {
StateFnFakes state_fn_fakes;  // NOLINT
}
// NOLINTEND

extern "C" {
// void transition_fn(hsm4c_event_t e) { state_fn_mocks.transition_fn(e); }
// bool guard_fn(hsm4c_event_t e) { return state_fn_mocks.guard_fn(e); }
// hsm4c_target_state_t choice_fn(hsm4c_event_t e) { return state_fn_mocks.choice_fn(e); }

void entry_fn(void *ctx, hsm4c_state_t const *state) { state_fn_fakes.entry_fn(ctx, state); }
void exit_fn(void *ctx, hsm4c_state_t const *state) { state_fn_fakes.exit_fn(ctx, state); }
}

}  // namespace Fakes

/* -------- Tests -------- */

using trompeloeil::_;  // NOLINT

// TEST_CASE("Asserts", "[!shouldfail]") {
//   hsm4c_t sm;
//   REQUIRE(hsm4c_start(&sm, nullptr) == HSM4C_ERROR);
// }

TEST_CASE("Defect cfg's", "[hsm4c]") {
  hsm4c_t sm;

  SECTION("states is nullptr") {
    std::array<hsm4c_state_rt_t, 1> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = nullptr,
        .states_rt = states_rt.data(),
        .num_states = states_rt.size(),
        .ctx = nullptr,
    };
    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_ERROR);
  }

  SECTION("states_rt is nullptr") {
    std::array<hsm4c_state_t, 1> const states{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = nullptr,
        .num_states = states.size(),
        .ctx = nullptr,
    };
    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_ERROR);
  }

  SECTION("num_states is 0") {
    std::array<hsm4c_state_t, 1> const states{};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = 0,
        .ctx = nullptr,
    };
    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_ERROR);
  }
}

TEST_CASE("Start Single State", "[hsm4c]") {
  hsm4c_t sm;

  SECTION("Valid cfg") {
    std::array<hsm4c_state_t, 1> const states{};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };
    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);
  }

  SECTION("Only Entry called") {
    std::array<hsm4c_state_t, 1> const states = {{{
        .compound = {.entry_fn = Fakes::entry_fn, .exit_fn = Fakes::exit_fn},
    }}};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };

    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, _));
    FORBID_CALL(Fakes::state_fn_fakes, exit_fn(_, _));

    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);
  }
}

TEST_CASE("Start Hierachical Statemachine", "[hsm4c]") {
  hsm4c_t sm;

  SECTION("Only Entries called") {
    enum States : hsm4c_state_id_t {  // NOLINT(performance-enum-size)
      ROOT,
      R_1,
      R_1_1,
      R_1_2,
    };

    std::array<hsm4c_state_t, 4> const states = {{
        [States::ROOT] =
            {
                .compound = {.initial = 1, .entry_fn = Fakes::entry_fn, .exit_fn = Fakes::exit_fn},
            },
        [States::R_1] =
            {
                .parent = 0,
                .compound = {.initial = 2, .entry_fn = Fakes::entry_fn, .exit_fn = Fakes::exit_fn},
            },
        [States::R_1_1] =
            {
                .parent = 1,
                .compound = {.entry_fn = Fakes::entry_fn, .exit_fn = Fakes::exit_fn},
            },
        [States::R_1_2] =
            {
                // Part of the hierachy but not an initial state
                .parent = 1,
                .compound = {.entry_fn = Fakes::entry_fn, .exit_fn = Fakes::exit_fn},
            },
    }};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };

    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::ROOT]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1_1]));
    FORBID_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1_2]));

    FORBID_CALL(Fakes::state_fn_fakes, exit_fn(_, _));

    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1_1]);
  }
}

TEST_CASE("Single State Transitions") {
  hsm4c_t sm;

  SECTION("No transitions") {
    enum States : hsm4c_state_id_t {  // NOLINT(performance-enum-size)
      ROOT,
      R_1,
    };

    std::array<hsm4c_state_t, 4> const states = {{
        [States::ROOT] =
            {
                .compound =
                    {
                        .initial = States::R_1,
                        .entry_fn = Fakes::entry_fn,
                        .exit_fn = Fakes::exit_fn,
                    },
            },
        [States::R_1] =
            {
                .parent = 0,
                .compound =
                    {
                        .entry_fn = Fakes::entry_fn,
                        .exit_fn = Fakes::exit_fn,
                        .transitions =
                            {
                                .entries = nullptr,  // No transitions
                                .num = 0,
                            },
                    },
            },
    }};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };

    ALLOW_CALL(Fakes::state_fn_fakes, entry_fn(_, _));

    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);

    FORBID_CALL(Fakes::state_fn_fakes, entry_fn(_, _));

    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1]);
    REQUIRE(hsm4c_run2completion(&sm, {.normal.id = 0}) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1]);
  }

  SECTION("Normal transitions") {
    enum States : hsm4c_state_id_t {  // NOLINT(performance-enum-size)
      ROOT,
      R_1,
    };

    std::array<hsm4c_transition_t, 2> const transitions_r_1 = {{
        {
            .target = States::R_1,
            .trigger.normal.id = 0,
        },
        {
            .target = States::R_1,
            .trigger.normal.id = 1,
        },
    }};

    std::array<hsm4c_state_t, 4> const states = {{
        [States::ROOT] =
            {
                .compound =
                    {
                        .initial = States::R_1,
                        .entry_fn = Fakes::entry_fn,
                        .exit_fn = Fakes::exit_fn,
                    },
                .name = "ROOT",
            },
        [States::R_1] =
            {
                .parent = 0,
                .compound =
                    {
                        .entry_fn = Fakes::entry_fn,
                        .exit_fn = Fakes::exit_fn,
                        .transitions =
                            {
                                .entries = transitions_r_1.data(),
                                .num = transitions_r_1.size(),
                            },
                    },
                .name = "R_1",
            },
    }};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };

    ALLOW_CALL(Fakes::state_fn_fakes, entry_fn(_, _));

    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);

    REQUIRE_CALL(Fakes::state_fn_fakes, exit_fn(_, &states[States::R_1]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1]));

    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1]);
    REQUIRE(hsm4c_run2completion(&sm, {.normal.id = 0}) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1]);
  }
}

TEST_CASE("Hierachical Multi State Transitions") {
  hsm4c_t sm;

  SECTION("Normal transitions") {
    enum States : hsm4c_state_id_t {  // NOLINT(performance-enum-size)
      ROOT,
      R_1,
      R_1_1,
      R_1_2,
    };

    std::array<hsm4c_transition_t, 2> const transitions_r_1 = {{
        {
            .target = States::R_1_2,
            .trigger.normal.id = 0,
        },
        {
            .target = States::R_1_1,
            .trigger.normal.id = 1,
        },
    }};

    std::array<hsm4c_transition_t, 2> const transitions_r_2 = {{
        {
            .target = States::R_1_1,
            .trigger.normal.id = 0,
        },
        {
            .target = States::R_1_2,
            .trigger.normal.id = 1,
        },
    }};

    std::array<hsm4c_state_t, 4> const
        states =
            {
                {
                    [States::ROOT] =
                        {
                            .compound =
                                {
                                    .initial = States::R_1,
                                    .entry_fn = Fakes::entry_fn,
                                    .exit_fn = Fakes::exit_fn,
                                },
                            .name = "ROOT",
                        },
                        [States::R_1] =
                        {
                            .compound =
                                {
                                    .initial = States::R_1_1,
                                    .entry_fn = Fakes::entry_fn,
                                    .exit_fn = Fakes::exit_fn,
                                },
                            .name = "R_1",
                        },
                    [States::R_1_1] =
                        {
                            .compound =
                                {
                                    .entry_fn = Fakes::entry_fn,
                                    .exit_fn = Fakes::exit_fn,
                                    .transitions =
                                        {
                                            .entries = transitions_r_1.data(),
                                            .num = transitions_r_1.size(),
                                        },
                                },
                            .name = "R_1_1",
                        },
                    [States::R_1_2] =
                        {
                            .compound =
                                {
                                    .entry_fn = Fakes::entry_fn,
                                    .exit_fn = Fakes::exit_fn,
                                    .transitions =
                                        {
                                            .entries = transitions_r_2.data(),
                                            .num = transitions_r_2.size(),
                                        },
                                },
                            .name = "R_1_2",
                        },
                }};

    std::array<hsm4c_state_rt_t, states.size()> states_rt{};

    hsm4c_cfg_t const cfg = {
        .states = states.data(),
        .states_rt = states_rt.data(),
        .num_states = states.size(),
        .ctx = nullptr,
    };

    ALLOW_CALL(Fakes::state_fn_fakes, entry_fn(_, _));

    REQUIRE(hsm4c_start(&sm, &cfg) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1_1]);

    // R_1_1 -> R_1_2
    REQUIRE_CALL(Fakes::state_fn_fakes, exit_fn(_, &states[States::R_1_1]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1_2]));
    REQUIRE(hsm4c_run2completion(&sm, {.normal.id = 0}) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1_2]);

    // R_1_2 -> R_1_2 (Self External Transition)
    REQUIRE_CALL(Fakes::state_fn_fakes, exit_fn(_, &states[States::R_1_2]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1_2]));
    REQUIRE(hsm4c_run2completion(&sm, {.normal.id = 1}) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1_2]);

    // R_1_2 -> R_1_1
    REQUIRE_CALL(Fakes::state_fn_fakes, exit_fn(_, &states[States::R_1_2]));
    REQUIRE_CALL(Fakes::state_fn_fakes, entry_fn(_, &states[States::R_1_1]));
    REQUIRE(hsm4c_run2completion(&sm, {.normal.id = 0}) == HSM4C_OK);
    REQUIRE(hsm4c_get_current_state(&sm) == &states[States::R_1_1]);
  }
}
