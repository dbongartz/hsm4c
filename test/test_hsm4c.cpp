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
#include "doctest.h"

#include "test_chart.h"

#include <map>
#include <string>
#include <vector>

using Log = std::vector<std::string>;

/* -------- FAKES -------- */

static Log calls;
static bool guard_return = true;
static std::map<State const *, State *> run_return;
static bool choice_A_return = false;
static int choice_A_called = 0;
static bool choice_B_return = false;
static int choice_B_called = 0;

static std::string name(State const *s) { return s->config->name; }

extern "C" {
void s_entry(State const *s) { calls.push_back("+" + name(s)); }
void s_exit(State const *s) { calls.push_back("-" + name(s)); }
State *s_run(State const *s, EventType event) {
  calls.push_back(name(s) + ".run(" + std::to_string(event) + ")");
  auto it = run_return.find(s);
  return it == run_return.end() ? nullptr : it->second;
}
bool t_guard(State const *) {
  calls.push_back("guard");
  return guard_return;
}
void t_action(State const *) { calls.push_back("act"); }
bool t_choice_A(State const *) {
  choice_A_called++;
  return choice_A_return;
}
bool t_choice_B(State const *) {
  choice_B_called++;
  return choice_B_return;
}
}

/* -------- FIXTURE -------- */

struct Fixture {
  State *const root = &states[ROOT];

  Fixture() {
    chart_reset();
    calls.clear();
    guard_return = true;
    run_return.clear();
    choice_A_return = choice_B_return = false;
    choice_A_called = choice_B_called = 0;
  }

  /** \brief Init the chart and run the given events without recording calls. */
  void prepare(std::vector<EventType> const &events = {}) {
    sc_init(root);
    for (EventType e : events) {
      sc_run(root, e);
    }
    calls.clear();
  }

  /** \brief Run one event and return the recorded calls. */
  Log run(EventType e) {
    calls.clear();
    last = sc_run(root, e);
    return calls;
  }

  State const *last = nullptr;
};

/** \brief Expected run calls for a branch, leaf first. */
static Log runs(std::vector<int> const &branch, EventType e) {
  Log l;
  for (int s : branch) {
    l.push_back(name(&states[s]) + ".run(" + std::to_string(e) + ")");
  }
  return l;
}

static Log operator+(Log a, Log const &b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}

/* -------- TESTS -------- */

TEST_CASE_FIXTURE(Fixture, "initial") {
  sc_init(root);
  CHECK(calls == Log{"+ROOT", "+A", "+AA", "+AAA"});

  // Double init to check if it resets accordingly
  calls.clear();
  sc_init(root);
  CHECK(calls == Log{"+ROOT", "+A", "+AA", "+AAA"});
}

TEST_CASE_FIXTURE(Fixture, "A to B") {
  prepare();
  CHECK(run(EV_1) == Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BA"} +
                         runs({BA, B, ROOT}, EV_1));
}

TEST_CASE_FIXTURE(Fixture, "B to A") {
  prepare({EV_1});
  CHECK(run(EV_1) == Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAA"} +
                         runs({AAA, AA, A, ROOT}, EV_1));
}

TEST_CASE_FIXTURE(Fixture, "A to BB") {
  prepare({EV_1, EV_1});
  CHECK(run(EV_2) == Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BB"} +
                         runs({BB, B, ROOT}, EV_2));
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
  CHECK(run(EV_4) == Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAA"} +
                         runs({AAA, AA, A, ROOT}, EV_4));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAB to B to A deep history") {
  // AAB, then B with A->AA->AAB deep history. Expecting AAB.
  prepare({EV_4, EV_4});
  CHECK(run(EV_5) == Log{"guard", "-BA", "-B", "act", "+A", "+AA", "+AAB"} +
                         runs({AAB, AA, A, ROOT}, EV_5));
}

TEST_CASE_FIXTURE(Fixture, "A to C via choice on later poll") {
  prepare();
  CHECK(run(EV_6) == Log{"guard", "-AAA", "-AA", "act"} + runs({A, ROOT}, EV_6));
  CHECK(choice_A_called == 1);
  CHECK(choice_B_called == 1);

  // Should go to C next poll with a non matching or SC_NO_EVENT
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
  CHECK(run(EV_7) == Log{"guard", "-AAA", "-AA", "-A", "act", "+B", "+BC"} +
                         runs({BC, B, ROOT}, EV_7));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAA external") {
  prepare();
  CHECK(run(EV_8) == Log{"guard", "-AAA", "act", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_8));
}

TEST_CASE_FIXTURE(Fixture, "AA to AAB external") {
  prepare();
  CHECK(run(EV_9) == Log{"guard", "-AAA", "-AA", "act", "+AA", "+AAB"} +
                         runs({AAB, AA, A, ROOT}, EV_9));
}

TEST_CASE_FIXTURE(Fixture, "AA to AAB local") {
  prepare();
  CHECK(run(EV_10) == Log{"guard", "-AAA", "act", "+AAB"} + runs({AAB, AA, A, ROOT}, EV_10));
}

TEST_CASE_FIXTURE(Fixture, "AAB to AA local") {
  prepare({EV_10});
  // Local child -> parent: AA is not exited/entered, only re-initialized.
  CHECK(run(EV_12) == Log{"guard", "-AAB", "act", "+AAA"} + runs({AAA, AA, A, ROOT}, EV_12));
}

TEST_CASE_FIXTURE(Fixture, "AAA to AAA internal") {
  prepare();
  CHECK(run(EV_11) == Log{"guard", "act"} + runs({AAA, AA, A, ROOT}, EV_11));
}

TEST_CASE_FIXTURE(Fixture, "A to A internal on composite state") {
  prepare();
  // Must not exit/enter any substate.
  CHECK(run(EV_13) == Log{"guard", "act"} + runs({AAA, AA, A, ROOT}, EV_13));
  CHECK(last == &states[AAA]);
}

TEST_CASE_FIXTURE(Fixture, "run returning own composite state is no change") {
  prepare();
  run_return[&states[A]] = &states[A];
  CHECK(run(EV_NO_EVENT) == runs({AAA, AA, A, ROOT}, EV_NO_EVENT));
  CHECK(last == &states[AAA]);
}

TEST_CASE_FIXTURE(Fixture, "false guard takes no transition") {
  prepare();
  guard_return = false;
  CHECK(run(EV_1) == Log{"guard"} + runs({AAA, AA, A, ROOT}, EV_1));
  CHECK(last == &states[AAA]);
}

/*
 * TODO:
 *
 * - Empty state and transition functions
 * - Parent state with no initial (Should work as target and with child as target)
 * - Auto transition on normal states
 * - Auto transitions on history states (Should not execute!)
 * - initial != self (can be cought)
 * - initial != other parent (can be cought)
 * - State change via run functions on lowest and high level
 * - State change via run on no transition found
 * - State change via run automatic
 */
