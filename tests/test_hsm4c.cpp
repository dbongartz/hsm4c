#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

#include "hsm4c.h"

#include "trompeloeil/mock.hpp"

#include <cstring>

using trompeloeil::_;

class StateFnMocks {
public:
  MAKE_MOCK1(entry_fn, void(hsm4c_state_t *s));
  MAKE_MOCK1(exit_fn, void(hsm4c_state_t *s));

  MAKE_MOCK1(transition_fn, void(hsm4c_event_t e));
  MAKE_MOCK1(guard_fn, bool(hsm4c_event_t e));
  MAKE_MOCK1(choice_fn, hsm4c_target_state_t(hsm4c_event_t e));
};

static StateFnMocks state_fn_mocks;

extern "C" {
void transition_fn(hsm4c_event_t e) { state_fn_mocks.transition_fn(e); }
bool guard_fn(hsm4c_event_t e) { return state_fn_mocks.guard_fn(e); }
hsm4c_target_state_t choice_fn(hsm4c_event_t e) { return state_fn_mocks.choice_fn(e); }

void entry_fn(hsm4c_state_t *s) { state_fn_mocks.entry_fn(s); }
void exit_fn(hsm4c_state_t *s) { state_fn_mocks.exit_fn(s); }
}

TEST_CASE("Minimal") {
  static hsm4c_state_t s1;
  static hsm4c_state_t s2;

  std::memset(&s1, 0, sizeof(s1));
  std::memset(&s2, 0, sizeof(s2));

  static hsm4c_transition_t const t_s1[] = {
      {0, {.fixed = {&s2}}},
  };
  static hsm4c_state_cfg_t const cfg_s1 = {
      .transitions = t_s1,
      .num_transitions = ARRAY_SIZE(t_s1),
      .name = "s1",
  };
  s1.cfg = &cfg_s1;

  static hsm4c_transition_t const t_s2[] = {
      {1, {.fixed = {&s1}}},
  };
  static hsm4c_state_cfg_t const cfg_s2 = {
      .transitions = t_s2,
      .num_transitions = ARRAY_SIZE(t_s2),
      .name = "s2",
  };
  s2.cfg = &cfg_s2;

  hsm4c_state_t *s = &s1;

  SECTION("init") {
    s = hsm4c_init(s);
    REQUIRE(&s1 == s);
  }

  SECTION("dispatch two states") {
    s = hsm4c_dispatch(s, (hsm4c_event_t){0});
    REQUIRE(&s2 == s);
    s = hsm4c_dispatch(s, (hsm4c_event_t){1});
    REQUIRE(&s1 == s);
  }

  SECTION("event not found") {
    s = hsm4c_dispatch(s, (hsm4c_event_t){2});
    REQUIRE(&s1 == s);
  }
}

TEST_CASE("Actions") {

  static hsm4c_state_t s1;
  static hsm4c_state_t s2;

  std::memset(&s1, 0, sizeof(s1));
  std::memset(&s2, 0, sizeof(s2));

  static hsm4c_transition_t const t_s1[] = {
      {0, {.fixed = {&s2}}, guard_fn, transition_fn},
  };
  static hsm4c_state_cfg_t const cfg_s1 = {
      .transitions = t_s1,
      .num_transitions = ARRAY_SIZE(t_s1),
      .name = "s1",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
  };
  s1.cfg = &cfg_s1;

  static hsm4c_transition_t const t_s2[] = {
      {1, {.fixed = {&s1}}, guard_fn, transition_fn},
  };
  static hsm4c_state_cfg_t const cfg_s2 = {
      .transitions = t_s2,
      .num_transitions = ARRAY_SIZE(t_s2),
      .name = "s2",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
  };
  s2.cfg = &cfg_s2;

  hsm4c_state_t *s = &s1;

  SECTION("init") {
    trompeloeil::sequence seq;
    REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);

    s = hsm4c_init(s);
    REQUIRE(&s1 == s);

    SECTION("dispatch two states") {
      trompeloeil::sequence seq;

      hsm4c_event_t const e0 = {0};
      hsm4c_event_t const e1 = {1};

      REQUIRE_CALL(state_fn_mocks, guard_fn(_)).RETURN(true).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&s1)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, transition_fn(_)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&s2)).IN_SEQUENCE(seq);
      s = hsm4c_dispatch(s, e0);
      REQUIRE(&s2 == s);

      REQUIRE_CALL(state_fn_mocks, guard_fn(_)).RETURN(true).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&s2)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, transition_fn(_)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);
      s = hsm4c_dispatch(s, e1);
      REQUIRE(&s1 == s);
    }

    SECTION("guard false") {
      trompeloeil::sequence seq;
      REQUIRE_CALL(state_fn_mocks, guard_fn(_)).RETURN(false).IN_SEQUENCE(seq);
      s = hsm4c_dispatch(s, (hsm4c_event_t){0});
      REQUIRE(&s1 == s);
    }
  }
}

#if HSM4C_CONFIG_HIERARCHICAL
TEST_CASE("Hierachical") {

  static hsm4c_state_t p1;
  static hsm4c_state_t p2;
  static hsm4c_state_t s1;
  static hsm4c_state_t s2;

  std::memset(&s1, 0, sizeof(s1));
  std::memset(&s2, 0, sizeof(s2));
  std::memset(&p1, 0, sizeof(p1));
  std::memset(&p2, 0, sizeof(p2));

  // static hsm4c_transition_t const t_p1[] = {
  //     {{0}, {.single = {&s1}}, guard_fn},
  // };
  static hsm4c_state_cfg_t const cfg_p1 = {
      .name = "p1",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
      .initial = &s1,
  };

  // static hsm4c_transition_t const t_p2[] = {
  //     {{0}, {.single = {&s2}}, guard_fn},
  // };
  static hsm4c_state_cfg_t const cfg_p2 = {
      .name = "p2",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
      .initial = &s2,
  };

  static hsm4c_transition_t const t_s1[] = {
      {0, {.fixed = {&s2}}, guard_fn, transition_fn},
  };
  static hsm4c_state_cfg_t const cfg_s1 = {
      .transitions = t_s1,
      .num_transitions = ARRAY_SIZE(t_s1),
      .name = "s1",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
      .parent = &p1,
  };

  static hsm4c_transition_t const t_s2[] = {
      {1, {.fixed = {&s1}}, guard_fn, transition_fn},
  };
  static hsm4c_state_cfg_t const cfg_s2 = {
      .transitions = t_s2,
      .num_transitions = ARRAY_SIZE(t_s2),
      .name = "s2",
      .entry_fn = entry_fn,
      .exit_fn = exit_fn,
      .parent = &p2,
  };

  p1.cfg = &cfg_p1;
  p2.cfg = &cfg_p2;
  s1.cfg = &cfg_s1;
  s2.cfg = &cfg_s2;

  hsm4c_state_t *s = &s1;

  SECTION("init") {
    trompeloeil::sequence seq;
    REQUIRE_CALL(state_fn_mocks, entry_fn(&p1)).IN_SEQUENCE(seq);
    REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);

    s = hsm4c_init(s);
    REQUIRE(&s1 == s);

    SECTION("dispatch two states") {
      trompeloeil::sequence seq;

      hsm4c_event_t const e0 = {0};
      hsm4c_event_t const e1 = {1};

      REQUIRE_CALL(state_fn_mocks, guard_fn(_)).RETURN(true).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&s1)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&p1)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, transition_fn(_)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&p2)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&s2)).IN_SEQUENCE(seq);
      s = hsm4c_dispatch(s, e0);
      REQUIRE(&s2 == s);

      REQUIRE_CALL(state_fn_mocks, guard_fn(_)).RETURN(true).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&s2)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, exit_fn(&p2)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, transition_fn(_)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&p1)).IN_SEQUENCE(seq);
      REQUIRE_CALL(state_fn_mocks, entry_fn(&s1)).IN_SEQUENCE(seq);
      s = hsm4c_dispatch(s, e1);
      REQUIRE(&s1 == s);
    }
  }
}
#endif
