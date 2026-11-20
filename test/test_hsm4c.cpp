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

#include <functional>
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
static hsm4c_event_t guard_event{};
static hsm4c_event_t action_event{};
static hsm4c_event_t run_event{};
static hsm4c_event_t x_auto_event{};
static hsm4c_result_t raise_result = HSM4C_OK;
static hsm4c_result_t reenter_result = HSM4C_OK;
/* Optional checks run from the entry, exit and action fakes */
static std::function<void(hsm4c_t *, hsm4c_state_id_t)> entry_probe;
static std::function<void(hsm4c_t *, hsm4c_state_id_t)> exit_probe;
static std::function<void(hsm4c_t *)> action_probe;

static std::string name(hsm4c_chart_t const &c, int s) { return c.states[s].name; }
static std::string name(hsm4c_t const *sm, hsm4c_state_id_t s) { return name(*sm->chart, s); }

extern "C" {
void s_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  calls.push_back("+" + name(sm, state));
  if (entry_probe) {
    entry_probe(sm, state);
  }
}
void s_exit(hsm4c_t *sm, hsm4c_state_id_t state) {
  calls.push_back("-" + name(sm, state));
  if (exit_probe) {
    exit_probe(sm, state);
  }
}
hsm4c_state_id_t s_run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_t const *event) {
  run_event = *event;
  calls.push_back(name(sm, state) + ".run(" + std::to_string(event->id) + ")");
  auto it = run_return.find(state);
  return it == run_return.end() ? HSM4C_NO_STATE : it->second;
}
bool t_guard(hsm4c_t const * /*sm*/, hsm4c_event_t const *event) {
  guard_event = *event;
  calls.push_back("guard");
  return guard_return;
}
void t_action(hsm4c_t *sm, hsm4c_event_t const *event) {
  action_ctx = hsm4c_ctx(sm);
  action_event = *event;
  if (action_probe) {
    action_probe(sm);
  }
  calls.push_back("act");
}
bool t_choice_A(hsm4c_t const * /*sm*/, hsm4c_event_t const * /*event*/) {
  choice_A_called++;
  return choice_A_return;
}
bool t_choice_B(hsm4c_t const * /*sm*/, hsm4c_event_t const * /*event*/) {
  choice_B_called++;
  return choice_B_return;
}
void x_raise(hsm4c_t *sm, hsm4c_event_t const * /*event*/) {
  calls.push_back("raise");
  raise_result = hsm4c_raise(sm, EV_8, nullptr);
  reenter_result = hsm4c_run(sm, EV_1, nullptr);
}
void x_ping(hsm4c_t *sm, hsm4c_event_t const * /*event*/) {
  raise_result = hsm4c_raise(sm, EV_13, nullptr);
}
bool x_auto(hsm4c_t const * /*sm*/, hsm4c_event_t const *event) {
  x_auto_event = *event;
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
    guard_event = action_event = run_event = x_auto_event = hsm4c_event_t{};
    raise_result = reenter_result = HSM4C_OK;
    entry_probe = exit_probe = nullptr;
    action_probe = nullptr;
  }

  /** \brief Init the chart and run the given events without recording calls. */
  void prepare(std::vector<hsm4c_event_id_t> const &events = {}) {
    CHECK(hsm4c_init(&sm, &chart, active, &ctx) == HSM4C_OK);
    for (hsm4c_event_id_t e : events) {
      CHECK(hsm4c_run(&sm, e, nullptr) == HSM4C_OK);
    }
    calls.clear();
  }

  /** \brief Run one event and return the recorded calls. */
  Log run(hsm4c_event_id_t e) {
    calls.clear();
    CHECK(hsm4c_run(&sm, e, nullptr) == HSM4C_OK);
    last = hsm4c_current(&sm);
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
  CHECK(hsm4c_init(&sm, &chart, active, &ctx) == HSM4C_OK);
  CHECK(calls == Log{"+ROOT", "+A", "+AA", "+AAA"});

  // Double init to check if it resets accordingly
  calls.clear();
  CHECK(hsm4c_init(&sm, &chart, active, &ctx) == HSM4C_OK);
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

TEST_CASE_FIXTURE(Fixture, "states are active during their entry and exit functions") {
  std::vector<std::string> seen;
  auto probe = [&](char const *when, hsm4c_t *m, hsm4c_state_id_t state) {
    seen.push_back(std::string(when) + name(m, state) +
                   (hsm4c_is_active(m, state) ? " active" : "") + " in " +
                   name(m, hsm4c_current(m)));
  };
  entry_probe = [&](hsm4c_t *m, hsm4c_state_id_t s) { probe("+", m, s); };
  exit_probe = [&](hsm4c_t *m, hsm4c_state_id_t s) { probe("-", m, s); };
  action_probe = [&](hsm4c_t *m) { seen.push_back("act in " + name(m, hsm4c_current(m))); };

  CHECK(hsm4c_init(&sm, &chart, active, &ctx) == HSM4C_OK);
  CHECK(seen == std::vector<std::string>{"+ROOT active in ROOT", "+A active in A",
                                         "+AA active in AA", "+AAA active in AAA"});

  seen.clear();
  run(EV_1); // A -> B
  CHECK(seen == std::vector<std::string>{"-AAA active in AAA", "-AA active in AA", "-A active in A",
                                         "act in ROOT", "+B active in B", "+BA active in BA"});
}

TEST_CASE_FIXTURE(Fixture, "callbacks get the event and its data") {
  prepare();
  int payload = 0;
  CHECK(hsm4c_run(&sm, EV_1, &payload) == HSM4C_OK);
  CHECK(guard_event.id == EV_1);
  CHECK(guard_event.data == &payload);
  CHECK(action_event.id == EV_1);
  CHECK(action_event.data == &payload);
  CHECK(run_event.id == EV_1);
  CHECK(run_event.data == &payload);
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
  run_return[AA] = AAA; // The current leaf is also no change
  CHECK(run(EV_NO_EVENT) == runs({AAA, AA, A, ROOT}, EV_NO_EVENT));
  CHECK(last == AAA);
}

TEST_CASE_FIXTURE(Fixture, "false guard takes no transition") {
  prepare();
  guard_return = false;
  CHECK(run(EV_1) == Log{"guard"} + runs({AAA, AA, A, ROOT}, EV_1));
  CHECK(last == AAA);
}

TEST_CASE_FIXTURE(Fixture, "run functions can trigger transitions") {
  prepare();
  run_return[AAA] = AB;
  CHECK(run(EV_NO_EVENT) ==
        runs({AAA}, EV_NO_EVENT) + Log{"-AAA", "-AA", "+AB"} + runs({AB, A, ROOT}, EV_NO_EVENT));
  CHECK(last == AB);
}

TEST_CASE_FIXTURE(Fixture, "endless run function transitions stop at the limit") {
  prepare();
  run_return[AAA] = AAB;
  run_return[AAB] = AAA;
  CHECK(hsm4c_run(&sm, HSM4C_NO_EVENT, nullptr) == HSM4C_ERR_LIMIT);
  CHECK(hsm4c_is_active(&sm, hsm4c_current(&sm)));
}

/* -------- SCXML SEMANTICS -------- */

struct XFixture : Fixture {
  hsm4c_state_id_t xactive[NUM_XSTATES]{};

  void xprepare() {
    CHECK(hsm4c_init(&sm, &xchart, xactive, &ctx) == HSM4C_OK);
    calls.clear();
  }

  Log xrun(hsm4c_event_id_t e) {
    calls.clear();
    CHECK(hsm4c_run(&sm, e, nullptr) == HSM4C_OK);
    last = hsm4c_current(&sm);
    return calls;
  }
};

TEST_CASE_FIXTURE(XFixture, "init takes automatic transitions") {
  x_auto_return = true;
  CHECK(hsm4c_init(&sm, &xchart, xactive, &ctx) == HSM4C_OK);
  CHECK(hsm4c_current(&sm) == XR);
  // No run functions on init
  CHECK(calls == Log{"+XROOT", "+XP", "+XPA", "auto", "-XPA", "-XP", "+XR"});
}

TEST_CASE_FIXTURE(XFixture, "automatic transitions get no event during init") {
  CHECK(hsm4c_init(&sm, &xchart, xactive, &ctx) == HSM4C_OK);
  CHECK(x_auto_event.id == HSM4C_NO_EVENT);
  CHECK(x_auto_event.data == nullptr);
}

TEST_CASE_FIXTURE(XFixture, "automatic transitions get the processed event") {
  xprepare();
  x_auto_return = true;
  int payload = 0;
  CHECK(hsm4c_run(&sm, EV_4, &payload) == HSM4C_OK);
  CHECK(hsm4c_current(&sm) == XR);
  CHECK(x_auto_event.id == EV_4);
  CHECK(x_auto_event.data == &payload);
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

TEST_CASE_FIXTURE(XFixture, "endless automatic transitions stop at the limit") {
  xprepare();
  CHECK(hsm4c_run(&sm, EV_6, nullptr) == HSM4C_ERR_LIMIT);

  // XPA -> XL1 for the event, then XL1 <-> XL2 until HSM4C_MAX_TRANSITIONS automatic
  // transitions are taken
  int entries = 0;
  for (auto const &c : calls) {
    entries += (c == "+XL1" || c == "+XL2") ? 1 : 0;
  }
  CHECK(entries == HSM4C_MAX_TRANSITIONS + 1);

  // Consistent state: the reached state is active, the machine keeps working
  hsm4c_state_id_t const reached = hsm4c_current(&sm);
  CHECK((reached == XL1 || reached == XL2));
  CHECK(hsm4c_is_active(&sm, reached));
  CHECK(hsm4c_run(&sm, HSM4C_NO_EVENT, nullptr) == HSM4C_ERR_LIMIT);
}

TEST_CASE_FIXTURE(XFixture, "internal events are processed in the same call") {
  xprepare();
  CHECK(xrun(EV_7) ==
        Log{"-XPA", "raise", "+XPB", "-XPB", "-XP", "+XR"} + xruns({XR, XROOT}, EV_8));
  CHECK(raise_result == HSM4C_OK);
}

TEST_CASE_FIXTURE(XFixture, "hsm4c_run from a callback is refused") {
  xprepare();
  xrun(EV_7);
  CHECK(reenter_result == HSM4C_ERR_BUSY);
  CHECK(last == XR); // The nested EV_1 had no effect
}

TEST_CASE_FIXTURE(XFixture, "events raised outside a call come before the next event") {
  xprepare();
  CHECK(hsm4c_raise(&sm, EV_1, nullptr) == HSM4C_OK);
  // EV_1 (XPA -> XPB) first, then EV_2 has no transition in XPB
  CHECK(xrun(EV_2) == Log{"auto", "guard", "-XPA", "act", "+XPB"} + xruns({XPB, XP, XROOT}, EV_2));
  CHECK(last == XPB);
}

TEST_CASE_FIXTURE(XFixture, "internal events without transition are discarded") {
  xprepare();
  CHECK(hsm4c_raise(&sm, EV_4, nullptr) == HSM4C_OK);
  // XPA's automatic transition is checked before and after the queued event, and again after the
  // polled event
  CHECK(xrun(HSM4C_NO_EVENT) ==
        Log{"auto", "auto", "auto"} + xruns({XPA, XP, XROOT}, HSM4C_NO_EVENT));
  CHECK(sm.queue_len == 0);
  CHECK(last == XPA);
}

TEST_CASE_FIXTURE(XFixture, "raising into a full queue fails") {
  xprepare();
  for (int i = 0; i < HSM4C_EVENT_QUEUE_SIZE; ++i) {
    CHECK(hsm4c_raise(&sm, EV_4, nullptr) == HSM4C_OK);
  }
  CHECK(hsm4c_raise(&sm, EV_4, nullptr) == HSM4C_ERR_QUEUE_FULL);
}

TEST_CASE_FIXTURE(XFixture, "deep history without history falls back to the parent's initial") {
  xprepare();
  CHECK(xrun(EV_9) == Log{"-XPA", "-XP", "+XS", "+XSA"} + xruns({XSA, XS, XROOT}, EV_9));
}

TEST_CASE_FIXTURE(XFixture, "targetless transition without action changes nothing") {
  xprepare();
  CHECK(xrun(EV_11) == Log{"auto"} + xruns({XPA, XP, XROOT}, EV_11));
  CHECK(last == XPA);
}

TEST_CASE_FIXTURE(XFixture, "endless internal events stop at the limit and stay queued") {
  xprepare();
  xrun(EV_12); // XM1
  CHECK(hsm4c_raise(&sm, EV_13, nullptr) == HSM4C_OK);

  // Processed before the next event: each EV_13 transition raises the next EV_13
  CHECK(hsm4c_run(&sm, EV_1, nullptr) == HSM4C_ERR_LIMIT);
  CHECK(sm.queue_len == 1);
  CHECK((hsm4c_current(&sm) == XM1 || hsm4c_current(&sm) == XM2));
  CHECK(hsm4c_run(&sm, EV_1, nullptr) == HSM4C_ERR_LIMIT);
}

/* -------- VALIDATION -------- */

namespace {

using States = std::vector<hsm4c_state_t>;

constexpr hsm4c_transition_t table_end{0, 0, nullptr, nullptr, HSM4C_TRANSITION_TABLE_END};

hsm4c_state_t state(hsm4c_state_id_t parent, hsm4c_state_id_t initial = HSM4C_NO_STATE,
                    hsm4c_state_type_t type = HSM4C_STATE_NORMAL) {
  hsm4c_state_t s{};
  s.parent = parent;
  s.initial = initial;
  s.type = type;
  return s;
}

/** \brief ROOT (initial A), A, B */
States valid_states() { return {state(ROOT, 1), state(ROOT), state(ROOT)}; }

/** \brief Validates states and returns the reported invalid state, or -1 if valid. */
int invalid_state(States const &states) {
  hsm4c_chart_t const c{states.data(), static_cast<hsm4c_state_id_t>(states.size())};
  hsm4c_state_id_t bad = 0;
  return hsm4c_validate(&c, &bad) == HSM4C_OK ? -1 : bad;
}

} // namespace

TEST_CASE("test charts are valid") {
  CHECK(hsm4c_validate(&chart, nullptr) == HSM4C_OK);
  CHECK(hsm4c_validate(&xchart, nullptr) == HSM4C_OK);
  CHECK(invalid_state(valid_states()) == -1);
}

TEST_CASE("empty charts are invalid") {
  States const s = valid_states();
  hsm4c_chart_t const no_states{nullptr, 3};
  hsm4c_chart_t const zero_states{s.data(), 0};
  hsm4c_state_id_t bad = 42;
  CHECK(hsm4c_validate(&no_states, &bad) == HSM4C_ERR_INVALID_CHART);
  CHECK(bad == HSM4C_ROOT);
  CHECK(hsm4c_validate(&zero_states, nullptr) == HSM4C_ERR_INVALID_CHART);
  CHECK(hsm4c_validate(nullptr, nullptr) == HSM4C_ERR_INVALID_CHART);
}

TEST_CASE("parents must exist and reach the root") {
  States s = valid_states();
  s[2].parent = 7;
  CHECK(invalid_state(s) == 2);

  s = valid_states();
  s[1].parent = 2;
  s[2].parent = 1;
  CHECK(invalid_state(s) == 1);
}

TEST_CASE("pseudo states have no children") {
  States s = valid_states();
  s[2] = state(ROOT, HSM4C_NO_STATE, HSM4C_STATE_HISTORY);
  s.push_back(state(2));
  CHECK(invalid_state(s) == 3);
}

TEST_CASE("initial states are direct children and not histories") {
  States s = valid_states();
  s.push_back(state(1)); // 3, below A
  s[0].initial = 3;
  CHECK(invalid_state(s) == 0);

  s = valid_states();
  s[2] = state(ROOT, 1, HSM4C_STATE_HISTORY);
  s[0].initial = 2;
  CHECK(invalid_state(s) == 0);
  s[2].type = HSM4C_STATE_HISTORY_DEEP;
  CHECK(invalid_state(s) == 0);

  s = valid_states();
  s[0].initial = 9;
  CHECK(invalid_state(s) == 0);
}

TEST_CASE("the root is a normal state") {
  States s = valid_states();
  s[0].type = HSM4C_STATE_HISTORY;
  CHECK(invalid_state(s) == 0);
}

TEST_CASE("state types must be valid") {
  States s = valid_states();
  // Invalid on purpose
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  s[1].type = static_cast<hsm4c_state_type_t>(42);
  CHECK(invalid_state(s) == 1);
}

TEST_CASE("histories need a fallback below their parent") {
  States s = valid_states();
  s.push_back(state(1, HSM4C_NO_STATE, HSM4C_STATE_HISTORY_DEEP)); // 3: history of A
  CHECK(invalid_state(s) == 3);                                    // A has no initial

  s[3].initial = 2; // B is not below A
  CHECK(invalid_state(s) == 3);

  s.push_back(state(1)); // 4, below A
  s[3].initial = 4;
  CHECK(invalid_state(s) == -1);

  static hsm4c_transition_t const automatic[] = {
      {HSM4C_NO_EVENT, 1, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL}, table_end};
  s.push_back(state(1, HSM4C_NO_STATE, HSM4C_STATE_CHOICE)); // 5, below A
  s[5].transitions = automatic;
  s[3].initial = 5; // Not a normal state
  CHECK(invalid_state(s) == 3);
  s[3].initial = 4;

  s[3].initial = HSM4C_NO_STATE;
  s[1].initial = 4; // Falls back to A's initial
  CHECK(invalid_state(s) == -1);
}

TEST_CASE("pseudo states have no callbacks, histories no transitions") {
  States s = valid_states();
  s[2] = state(ROOT, 1, HSM4C_STATE_HISTORY);
  CHECK(invalid_state(s) == -1);

  s[2].entry_fn = s_entry;
  CHECK(invalid_state(s) == 2);
  s[2].entry_fn = nullptr;
  s[2].exit_fn = s_exit;
  CHECK(invalid_state(s) == 2);
  s[2].exit_fn = nullptr;
  s[2].run_fn = s_run;
  CHECK(invalid_state(s) == 2);
  s[2].run_fn = nullptr;

  static hsm4c_transition_t const empty[] = {table_end};
  s[2].transitions = empty;
  CHECK(invalid_state(s) == -1);

  static hsm4c_transition_t const t[] = {{EV_1, 1, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL},
                                         table_end};
  s[2].transitions = t;
  CHECK(invalid_state(s) == 2);
}

TEST_CASE("choices need automatic transitions") {
  static hsm4c_transition_t const automatic[] = {
      {HSM4C_NO_EVENT, 1, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL}, table_end};
  static hsm4c_transition_t const event[] = {{EV_1, 1, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL},
                                             table_end};
  static hsm4c_transition_t const empty[] = {table_end};

  States s = valid_states();
  s[2] = state(ROOT, HSM4C_NO_STATE, HSM4C_STATE_CHOICE);
  CHECK(invalid_state(s) == 2);
  s[2].transitions = empty;
  CHECK(invalid_state(s) == 2);
  s[2].transitions = event;
  CHECK(invalid_state(s) == 2);
  s[2].transitions = automatic;
  CHECK(invalid_state(s) == -1);
}

TEST_CASE("transitions must be valid") {
  static hsm4c_transition_t const bad_target[] = {
      {EV_1, 99, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL}, table_end};
  static hsm4c_transition_t const bad_event[] = {
      {-1, 1, nullptr, nullptr, HSM4C_TRANSITION_EXTERNAL}, table_end};
  // Invalid on purpose
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  auto const invalid_type = static_cast<hsm4c_transition_type_t>(42);
  static hsm4c_transition_t const bad_type[] = {{EV_1, 1, nullptr, nullptr, invalid_type},
                                                table_end};

  for (auto const *table : {bad_target, bad_event, bad_type}) {
    States s = valid_states();
    s[2].transitions = table;
    CHECK(invalid_state(s) == 2);
  }
}

TEST_CASE("init rejects invalid charts") {
  States s = valid_states();
  s[2].parent = 7;
  hsm4c_chart_t const c{s.data(), static_cast<hsm4c_state_id_t>(s.size())};
  hsm4c_t sm{};
  hsm4c_state_id_t active[3]{};
  CHECK(hsm4c_init(&sm, &c, active, nullptr) == HSM4C_ERR_INVALID_CHART);
}

TEST_CASE_FIXTURE(Fixture, "run functions returning unknown states are an error") {
  prepare();
  run_return[A] = 200;
  CHECK(hsm4c_run(&sm, HSM4C_NO_EVENT, nullptr) == HSM4C_ERR_INVALID_STATE);
  CHECK(hsm4c_current(&sm) == AAA);
}

/*
 * TODO:
 *
 * - Empty state and transition functions
 * - Parent state with no initial (Should work as target and with child as target)
 * - Auto transitions on history states (Should not execute!)
 * - hsm4c_state_t change via run functions on lowest and high level
 * - hsm4c_state_t change via run on no transition found
 * - hsm4c_state_t change via run automatic
 */
