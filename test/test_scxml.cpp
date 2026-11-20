/**
 * SCXML conformance: charts ported from the W3C SCXML IRP test suite, see scxml_charts.c.
 * Each test runs a chart and checks that it ends in its pass state.
 */

#include <doctest/doctest.h>

#include "scxml_charts.h"

#include <vector>

namespace {

/** \brief Runs a chart: hsm4c_init(), then the given external events. Returns the final state. */
hsm4c_state_id_t run_chart(hsm4c_chart_t const &chart,
                           std::vector<hsm4c_event_id_t> const &external = {}) {
  scxml_ctx ctx{};
  hsm4c_t sm{};
  std::vector<hsm4c_state_id_t> active(chart.num_states);
  REQUIRE(hsm4c_init(&sm, &chart, active.data(), &ctx) == HSM4C_OK);
  for (hsm4c_event_id_t const e : external) {
    REQUIRE(hsm4c_run(&sm, e, nullptr) == HSM4C_OK);
  }
  return hsm4c_current(&sm);
}

} // namespace

TEST_CASE("W3C 387: default history configuration") { CHECK(run_chart(scxml_387) == SCXML_PASS); }

TEST_CASE("W3C 388: stored history configuration") { CHECK(run_chart(scxml_388) == SCXML_PASS); }

TEST_CASE("W3C 403a: transition selection, child before parent, document order") {
  CHECK(run_chart(scxml_403a) == SCXML_PASS);
}

TEST_CASE("W3C 407: exit functions are called") { CHECK(run_chart(scxml_407) == SCXML_PASS); }

TEST_CASE("W3C 411: states are active before their entry functions") {
  CHECK(run_chart(scxml_411) == SCXML_PASS);
}

TEST_CASE("W3C 419: eventless transitions before events") {
  CHECK(run_chart(scxml_419, {SCXML_EXTERNAL}) == SCXML_PASS);
}

TEST_CASE("W3C 421: internal events before external events") {
  CHECK(run_chart(scxml_421, {SCXML_EXTERNAL}) == SCXML_PASS);
}

TEST_CASE("W3C 503: targetless transitions do not exit their source") {
  CHECK(run_chart(scxml_503) == SCXML_PASS);
}

TEST_CASE("W3C 505: local transitions do not exit their source") {
  CHECK(run_chart(scxml_505) == SCXML_PASS);
}

TEST_CASE("W3C 506: local transitions to a non-descendant behave as external") {
  CHECK(run_chart(scxml_506) == SCXML_PASS);
}

TEST_CASE("W3C 580: history states never become active") {
  CHECK(run_chart(scxml_580) == SCXML_PASS);
}
