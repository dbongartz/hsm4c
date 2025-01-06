#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "hsm4c.h"

#include "trompeloeil/mock.hpp"

using trompeloeil::_;

class StateFnMocks {
public:
  MAKE_MOCK1(entry_fn, void(hsm4c_state_t *s));
  MAKE_MOCK1(exit_fn, void(hsm4c_state_t *s));

  // MAKE_MOCK1(transition_fn, void(hsm4c_event_t e));
  // MAKE_MOCK1(guard_fn, bool(hsm4c_event_t e));
  // MAKE_MOCK1(choice_fn, hsm4c_target_state_t(hsm4c_event_t e));
};

static StateFnMocks state_fn_mocks;

extern "C" {
// void transition_fn(hsm4c_event_t e) { state_fn_mocks.transition_fn(e); }
// bool guard_fn(hsm4c_event_t e) { return state_fn_mocks.guard_fn(e); }
// hsm4c_target_state_t choice_fn(hsm4c_event_t e) { return state_fn_mocks.choice_fn(e); }

void entry_fn(hsm4c_state_t *s) { state_fn_mocks.entry_fn(s); }
void exit_fn(hsm4c_state_t *s) { state_fn_mocks.exit_fn(s); }
}

TEST_CASE("Minimal") {
}
