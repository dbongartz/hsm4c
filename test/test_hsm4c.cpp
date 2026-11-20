/**
 * Unit tests for hsm4c. See test_hsm4c_statechart.puml for the tested statechart.
 *
 * The state and transition functions are recording fakes: every call is appended to a log,
 * and each test compares the log against the expected call sequence.
 *
 * Log format:
 *   "+X"        entry of X        "-X"   exit of X
 *   "X.run(e)"  run of X with e   "guard" / "act"  transition guard / action
 * Choice guards are not logged, they count their calls instead.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "test_chart.h"

#include <map>
#include <string>
#include <vector>

using Log = std::vector<std::string>;

/* -------- FAKES -------- */

static Log calls;
static bool guard_return = true;
static std::map<hsm4c_state_id_t, hsm4c_state_id_t> run_return;
static bool choice_A_return = false;
static int choice_A_called = 0;
static bool choice_B_return = false;
static int choice_B_called = 0;
static bool x_auto_return = false;
static void *action_ctx = nullptr;

static std::string name(hsm4c_chart_t const &c, int s) { return c.states[s].name; }
static std::string name(hsm4c_t const *sm, hsm4c_state_id_t s) { return name(*sm->chart, s); }

extern "C" {
void s_entry(hsm4c_t *sm, hsm4c_state_id_t state) { calls.push_back("+" + name(sm, state)); }
void s_exit(hsm4c_t *sm, hsm4c_state_id_t state) { calls.push_back("-" + name(sm, state)); }
hsm4c_state_id_t s_run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_id_t event) {
  calls.push_back(name(sm, state) + ".run(" + std::to_string(event) + ")");
  auto it = run_return.find(state);
  return it == run_return.end() ? HSM4C_NO_STATE : it->second;
}
bool t_guard(hsm4c_t const * /*sm*/) {
  calls.push_back("guard");
  return guard_return;
}
void t_action(hsm4c_t *sm) {
  action_ctx = hsm4c_ctx(sm);
  calls.push_back("act");
}
bool t_choice_A(hsm4c_t const * /*sm*/) {
  choice_A_called++;
  return choice_A_return;
}
bool t_choice_B(hsm4c_t const * /*sm*/) {
  choice_B_called++;
  return choice_B_return;
}
bool x_auto(hsm4c_t const * /*sm*/) {
  calls.push_back("auto");
  return x_auto_return;
}
}

/* -------- FIXTURE -------- */

struct Fixture {
  hsm4c_t sm{};
  hsm4c_state_id_t active[NUM_STATES]{};
  int ctx = 0;

  Fixture() {
    calls.clear();
    guard_return = true;
    run_return.clear();
    choice_A_return = choice_B_return = false;
    choice_A_called = choice_B_called = 0;
    x_auto_return = false;
    action_ctx = nullptr;
  }

  /** \brief Init the chart and run the given events without recording calls. */
  void prepare(std::vector<hsm4c_event_id_t> const &events = {}) {
    hsm4c_init(&sm, &chart, active, &ctx);
    for (hsm4c_event_id_t e : events) {
      hsm4c_run(&sm, e);
    }
    calls.clear();
  }

  /** \brief Run one event and return the recorded calls. */
  Log run(hsm4c_event_id_t e) {
    calls.clear();
    last = hsm4c_run(&sm, e);
    return calls;
  }

  hsm4c_state_id_t last = HSM4C_NO_STATE;
};

/** \brief Expected run calls for a branch, leaf first. */
static Log runs(std::vector<int> const &branch, hsm4c_event_id_t e,
                hsm4c_chart_t const &c = chart) {
  Log l;
  for (int s : branch) {
    l.push_back(name(c, s) + ".run(" + std::to_string(e) + ")");
  }
  return l;
}

/** \brief Same as runs() for the SCXML semantics chart. */
static Log xruns(std::vector<int> const &branch, hsm4c_event_id_t e) {
  return runs(branch, e, xchart);
}

static Log operator+(Log a, Log const &b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}

/* -------- TESTS -------- */

TEST_CASE_FIXTURE(Fixture, "initial") {
  hsm4c_init(&sm, &chart, active, &ctx);
  CHECK(calls == Log{"+ROOT", "+A", "+AA", "+AAA"});

  // Double init to check if it resets accordingly
  calls.clear();
  hsm4c_init(&sm, &chart, active, &ctx);
  CHECK(calls == Log{"+ROOT", "+A", "+AA", "+AAA"});
}

TEST_CASE_FIXTURE(Fixture, "A to B") {
  prepare();
  CHECK(run(EV_1) ==
        Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BA"} + runs({BA, B, ROOT}, EV_1));
}

TEST_CASE_FIXTURE(Fixture, "callbacks get the user context") {
  prepare();
  run(EV_1);
  CHECK(action_ctx == &ctx);
}

TEST_CASE_FIXTURE(Fixture, "B to A") {
  prepare({EV_1});
  CHECK(run(EV_1) ==
        Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_1));
}

TEST_CASE_FIXTURE(Fixture, "A to BB") {
  prepare({EV_1, EV_1});
  CHECK(run(EV_2) ==
        Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BB"} + runs({BB, B, ROOT}, EV_2));
}

TEST_CASE_FIXTURE(Fixture, "AA to AB") {
  prepare();
  CHECK(run(EV_3) == Log{"guard", "-AAA", "-AA", "act", "+AB"} + runs({AB, A, ROOT}, EV_3));
}

TEST_CASE_FIXTURE(Fixture, "AA to AB to B to A history") {
  prepare({EV_3});
  // Now in A->AB
  CHECK(run(EV_3) == Log{"guard", "-AB", "-A", "act", "+B", "+BA"} + runs({BA, B, ROOT}, EV_3));
  // Now in B with A->AB history. We expect to land back in AB
  CHECK(run(EV_3) == Log{"guard", "-BA", "-B", "act", "+A", "+AB"} + runs({AB, A, ROOT}, EV_3));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAB") {
  prepare();
  CHECK(run(EV_4) == Log{"guard", "-AAA", "act", "+AAB"} + runs({AAB, AA, A, ROOT}, EV_4));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAB to B to A history") {
  // AAB, then B with A->AA->AAB history. Shallow history resets AA, expecting AAA.
  prepare({EV_4, EV_4});
  CHECK(run(EV_4) ==
        Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_4));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAB to B to A deep history") {
  // AAB, then B with A->AA->AAB deep history. Expecting AAB.
  prepare({EV_4, EV_4});
  CHECK(run(EV_5) ==
        Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAB"} + runs({AAB, AA, A, ROOT}, EV_5));
}

// All choice guards false: waits in the choice state and re-evaluates on the next hsm4c_run().
TEST_CASE_FIXTURE(Fixture, "A to C via choice on later poll") {
  prepare();
  CHECK(run(EV_6) == Log{"guard", "-AAA", "-AA", "act"} + runs({A, ROOT}, EV_6));
  CHECK(choice_A_called == 1);
  CHECK(choice_B_called == 1);

  // Should go to C next poll with a non matching or HSM4C_NO_EVENT
  choice_B_return = true;
  CHECK(run(EV_6) == Log{"-A", "act", "+C"} + runs({C, ROOT}, EV_6));
}

TEST_CASE_FIXTURE(Fixture, "A to B via choice immediately") {
  prepare();
  choice_A_return = true;
  CHECK(run(EV_6) == Log{"guard", "-AAA", "-AA", "act", // AA -> A_CHOICE
                         "-A", "act",                   // A_CHOICE -> B
                         "+B", "+BA"} +
                         runs({BA, B, ROOT}, EV_6));
  CHECK(choice_A_called == 1);
  CHECK(choice_B_called == 0);
}

TEST_CASE_FIXTURE(Fixture, "A to B history with no history set") {
  prepare();
  CHECK(run(EV_7) ==
        Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BC"} + runs({BC, B, ROOT}, EV_7));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAA external") {
  prepare();
  CHECK(run(EV_8) == Log{"guard", "-AAA", "act", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_8));
}

TEST_CASE_FIXTURE(Fixture, "AA to AAB external") {
  prepare();
  CHECK(run(EV_9) ==
        Log{"guard", "-AAA", "-AA", "act", "+AA", "+AAB"} + runs({AAB, AA, A, ROOT}, EV_9));
}

TEST_CASE_FIXTURE(Fixture, "AA to AAB local") {
  prepare();
  CHECK(run(EV_10) == Log{"guard", "-AAA", "act", "+AAB"} + runs({AAB, AA, A, ROOT}, EV_10));
}

TEST_CASE_FIXTURE(Fixture, "AAB to AA local behaves as external") {
  prepare({EV_10});
  // Local only applies when the target is a descendant of the source (SCXML type="internal").
  CHECK(run(EV_12) ==
        Log{"guard", "-AAB", "-AA", "act", "+AA", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_12));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAA internal") {
  prepare();
  CHECK(run(EV_11) == Log{"guard", "act"} + runs({AAA, AA, A, ROOT}, EV_11));
}

TEST_CASE_FIXTURE(Fixture, "A to A internal on composite state") {
  prepare();
  // Must not exit/enter any substate.
  CHECK(run(EV_13) == Log{"guard", "act"} + runs({AAA, AA, A, ROOT}, EV_13));
  CHECK(last == AAA);
}

TEST_CASE_FIXTURE(Fixture, "run returning own composite state is no change") {
  prepare();
  run_return[A] = A;
  CHECK(run(EV_NO_EVENT) == runs({AAA, AA, A, ROOT}, EV_NO_EVENT));
  CHECK(last == AAA);
}

TEST_CASE_FIXTURE(Fixture, "false guard takes no transition") {
  prepare();
  guard_return = false;
  CHECK(run(EV_1) == Log{"guard"} + runs({AAA, AA, A, ROOT}, EV_1));
  CHECK(last == AAA);
}

/* -------- SCXML SEMANTICS -------- */

struct XFixture : Fixture {
  hsm4c_state_id_t xactive[NUM_XSTATES]{};

  void xprepare() {
    hsm4c_init(&sm, &xchart, xactive, &ctx);
    calls.clear();
  }

  Log xrun(hsm4c_event_id_t e) {
    calls.clear();
    last = hsm4c_run(&sm, e);
    return calls;
  }
};

TEST_CASE_FIXTURE(XFixture, "init takes automatic transitions") {
  x_auto_return = true;
  CHECK(hsm4c_init(&sm, &xchart, xactive, &ctx) == XR);
  // No run functions on init
  CHECK(calls == Log{"+XROOT", "+XP", "+XPA", "auto", "-XPA", "-XP", "+XR"});
}

TEST_CASE_FIXTURE(XFixture, "event transitions take priority over automatic transitions") {
  xprepare();
  x_auto_return = true;
  // XPA's automatic transition is listed first and enabled, but the event is processed first.
  CHECK(xrun(EV_2) == Log{"guard", "-XPA", "-XP", "act", "+XQ"} + xruns({XQ, XROOT}, EV_2));
}

TEST_CASE_FIXTURE(XFixture, "automatic transitions are checked after an event without transition") {
  xprepare();
  x_auto_return = true;
  CHECK(xrun(EV_4) == Log{"auto", "-XPA", "-XP", "+XR"} + xruns({XR, XROOT}, EV_4));
}

TEST_CASE_FIXTURE(XFixture, "deeper source takes priority") {
  xprepare();
  // XP has XP -> XQ on EV_1, but XPA -> XPB has the deeper source.
  CHECK(xrun(EV_1) == Log{"guard", "-XPA", "act", "+XPB"} + xruns({XPB, XP, XROOT}, EV_1));
}

TEST_CASE_FIXTURE(XFixture, "targetless transition only executes its action") {
  xprepare();
  CHECK(xrun(EV_3) == Log{"guard", "act", "auto"} + xruns({XPA, XP, XROOT}, EV_3));
  CHECK(last == XPA);
}

TEST_CASE_FIXTURE(XFixture, "transition with root as source does not exit the root") {
  xprepare();
  CHECK(xrun(EV_5) == Log{"guard", "-XPA", "-XP", "act", "+XQ"} + xruns({XQ, XROOT}, EV_5));
}

/*
 * TODO:
 *
 * - Empty state and transition functions
 * - Parent state with no initial (Should work as target and with child as target)
 * - Auto transitions on history states (Should not execute!)
 * - initial != self (can be caught)
 * - initial != other parent (can be caught)
 * - hsm4c_state_t change via run functions on lowest and high level
 * - hsm4c_state_t change via run on no transition found
 * - hsm4c_state_t change via run automatic
 */
