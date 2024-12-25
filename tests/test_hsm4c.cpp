#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>
#include <iostream>

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
}

TEST_CASE("One State", "[trivial]") {
  hsm4c_state_t root;
  hsm4c_state_t s1;

  hsm4c_transition_t const root_transitions[] = {
      {
          .from = &s1,
          .to = &s1,
          .event = 1,
          .transition_fn = transition_fn,
          .guard_fn = guard_fn,
          .type = HSM4C_TTYPE_EXTERNAL,
      },
      HSM4C_TRANSITIONS_END,
  };
  static hsm4c_state_config_t const root_cfg = {
      .name = "",
      .entry_fn = entry_fn,
      .run_fn = run_fn,
      .exit_fn = exit_fn,
      .parent = nullptr,
      .initial = &s1,
      .type = HSM4C_TYPE_ROOT,
      .transitions = root_transitions,
  };
  root.config = &root_cfg;

  static hsm4c_state_config_t const s1_cfg = {
      .name = "",
      .entry_fn = entry_fn,
      .run_fn = run_fn,
      .exit_fn = exit_fn,
      .parent = &root,
      .initial = nullptr,
      .type = HSM4C_TYPE_NORMAL,
      .transitions = nullptr,
  };
  s1.config = &s1_cfg;

  std::cout << "Address of root: " << &root << std::endl;
  std::cout << "Address of s1: " << &s1 << std::endl;

  trompeloeil::sequence seq;

  REQUIRE_CALL(state_fn_mocks, entry_fn(&root)).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);

  hsm4c_state_t const *s = hsm4c_init(&root);
  REQUIRE(s == &s1);

  REQUIRE_CALL(state_fn_mocks, guard_fn(&root)).RETURN(true).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, exit_fn(&s1)).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, transition_fn(&root)).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, run_fn(&s1, 1)).RETURN(nullptr).IN_SEQUENCE(seq);
  REQUIRE_CALL(state_fn_mocks, run_fn(&root, 1)).RETURN(nullptr).IN_SEQUENCE(seq);

  s = hsm4c_run(&root, 1);
  REQUIRE(s == &s1);
}
