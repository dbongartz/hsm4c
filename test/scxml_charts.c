/*
 * Charts ported from the W3C SCXML 1.0 Implementation Report (IRP) test suite,
 * https://www.w3.org/Voice/2013/scxml-irp/ (test<N>.txml). Each chart names its test and
 * assertion. Only tests that hsm4c can express are ported: no parallel or final states, no
 * datamodel or invoke features.
 *
 * Common adaptations:
 * - SCXML's event="*" (any event) does not exist in hsm4c. It is replaced by rows for the other
 *   events the chart can produce.
 * - <send event="timeout"> guards against hangs. hsm4c_run() returns, so they are dropped.
 * - Counters (conf:incrementID, conf:idVal) live in struct scxml_ctx.
 * - States without 'initial' in hsm4c are leaves: SCXML's default "first child" is written out.
 *
 * State 0 is the root, SCXML_PASS and SCXML_FAIL are states 1 and 2 in every chart.
 */

#include "scxml_charts.h"

/* -------- Events -------- */

enum {
  EVENT1 = 1,
  EVENT2,
  EVENT3,
  EVENT4,
  FOO,
  BAR,
  INTERNAL,
  INTERNAL1,
  INTERNAL2,
  INTERNAL3,
  INTERNAL4,
};

/** \brief Event raised when entering a state, like the IRP tests' "enteringS011". */
#define ENTERING(state) (100 + (state))

/* -------- Callbacks -------- */

static int *var(hsm4c_t const *sm) { return ((struct scxml_ctx *)hsm4c_ctx(sm))->var; }

static bool var1_is_0(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[1] == 0;
}
static bool var1_is_1(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[1] == 1;
}
static bool var1_is_2(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[1] == 2;
}
static bool var1_is_3(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[1] == 3;
}
static bool var2_is_1(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[2] == 1;
}
static bool var2_is_2(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[2] == 2;
}
static bool var3_is_1(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return var(sm)[3] == 1;
}
static bool is_false(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)sm;
  (void)e;
  return false;
}

static void inc_var1(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  ++var(sm)[1];
}
static void inc_var2(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  ++var(sm)[2];
}
static void act_inc_var2(hsm4c_t *sm, hsm4c_event_t const *e) {
  (void)e;
  ++var(sm)[2];
}
static void act_inc_var3(hsm4c_t *sm, hsm4c_event_t const *e) {
  (void)e;
  ++var(sm)[3];
}

static void raise_entering(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)hsm4c_raise(sm, ENTERING(state), NULL);
}
static void raise_foo_bar(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  (void)hsm4c_raise(sm, FOO, NULL);
  (void)hsm4c_raise(sm, BAR, NULL);
}

#define ROW(ev, target)                                                                            \
  { .event = (ev), .to = (target) }
#define ROW_IF(ev, target, guard)                                                                  \
  { .event = (ev), .to = (target), .guard_fn = (guard) }
#define ROW_DO(ev, target, action)                                                                 \
  { .event = (ev), .to = (target), .action_fn = (action) }
#define AUTO(target) ROW(HSM4C_NO_EVENT, target)
#define AUTO_IF(target, guard) ROW_IF(HSM4C_NO_EVENT, target, guard)
#define END HSM4C_TRANSITIONS_END

/*
 * -------- 387 (3.10) --------
 * Before the parent state has been visited, a transition to a history state takes its default
 * stored configuration: shallow history of s0 enters s01 (initial s011), deep history of s1
 * enters s122.
 */

enum {
  T387_S0 = 3,
  T387_S0_HS,
  T387_S0_HD,
  T387_S01,
  T387_S011,
  T387_S012,
  T387_S02,
  T387_S021,
  T387_S022,
  T387_S1,
  T387_S1_HS,
  T387_S1_HD,
  T387_S11,
  T387_S111,
  T387_S112,
  T387_S12,
  T387_S121,
  T387_S122,
  T387_S3,
  T387_S4,
  T387_NUM
};

static hsm4c_transition_t const t387_s0[] = {
    ROW(ENTERING(T387_S011), T387_S4), ROW(ENTERING(T387_S012), SCXML_FAIL),
    ROW(ENTERING(T387_S021), SCXML_FAIL), ROW(ENTERING(T387_S022), SCXML_FAIL), END};
static hsm4c_transition_t const t387_s1[] = {
    ROW(ENTERING(T387_S122), SCXML_PASS), ROW(ENTERING(T387_S111), SCXML_FAIL),
    ROW(ENTERING(T387_S112), SCXML_FAIL), ROW(ENTERING(T387_S121), SCXML_FAIL), END};
static hsm4c_transition_t const t387_s3[] = {AUTO(T387_S0_HS), END};
static hsm4c_transition_t const t387_s4[] = {AUTO(T387_S1_HD), END};

static hsm4c_state_t const t387[T387_NUM] = {
    [HSM4C_ROOT] = {.initial = T387_S3},
    [T387_S0] = {.initial = T387_S01, .transitions = t387_s0},
    [T387_S0_HS] = {.parent = T387_S0, .initial = T387_S01, .type = HSM4C_STATE_HISTORY},
    [T387_S0_HD] = {.parent = T387_S0, .initial = T387_S022, .type = HSM4C_STATE_HISTORY_DEEP},
    [T387_S01] = {.parent = T387_S0, .initial = T387_S011},
    [T387_S011] = {.parent = T387_S01, .entry_fn = raise_entering},
    [T387_S012] = {.parent = T387_S01, .entry_fn = raise_entering},
    [T387_S02] = {.parent = T387_S0, .initial = T387_S021},
    [T387_S021] = {.parent = T387_S02, .entry_fn = raise_entering},
    [T387_S022] = {.parent = T387_S02, .entry_fn = raise_entering},
    [T387_S1] = {.initial = T387_S11, .transitions = t387_s1},
    [T387_S1_HS] = {.parent = T387_S1, .initial = T387_S11, .type = HSM4C_STATE_HISTORY},
    [T387_S1_HD] = {.parent = T387_S1, .initial = T387_S122, .type = HSM4C_STATE_HISTORY_DEEP},
    [T387_S11] = {.parent = T387_S1, .initial = T387_S111},
    [T387_S111] = {.parent = T387_S11, .entry_fn = raise_entering},
    [T387_S112] = {.parent = T387_S11, .entry_fn = raise_entering},
    [T387_S12] = {.parent = T387_S1, .initial = T387_S121},
    [T387_S121] = {.parent = T387_S12, .entry_fn = raise_entering},
    [T387_S122] = {.parent = T387_S12, .entry_fn = raise_entering},
    [T387_S3] = {.transitions = t387_s3},
    [T387_S4] = {.transitions = t387_s4},
};
hsm4c_chart_t const scxml_387 = HSM4C_CHART(t387);

/*
 * -------- 388 (3.10) --------
 * After the parent state has been visited, a transition to a history state takes the stored
 * configuration: deep history restores s012, shallow history restores s01 and resets it to its
 * initial s011. var1 counts the entries of s0. Adapted: the initial configuration (s012) is
 * reached by an automatic transition from a start state, hsm4c initial states are direct
 * children.
 */

enum {
  T388_START = 3,
  T388_S0,
  T388_S0_HS,
  T388_S0_HD,
  T388_S01,
  T388_S011,
  T388_S012,
  T388_S02,
  T388_S021,
  T388_S022,
  T388_S1,
  T388_S2,
  T388_NUM
};

static hsm4c_transition_t const t388_start[] = {AUTO(T388_S012), END};
static hsm4c_transition_t const t388_s0[] = {ROW_IF(ENTERING(T388_S012), T388_S1, var1_is_1),
                                             ROW_IF(ENTERING(T388_S012), T388_S2, var1_is_2),
                                             ROW_IF(ENTERING(T388_S011), SCXML_FAIL, var1_is_2),
                                             ROW_IF(ENTERING(T388_S021), SCXML_FAIL, var1_is_2),
                                             ROW_IF(ENTERING(T388_S022), SCXML_FAIL, var1_is_2),
                                             ROW_IF(ENTERING(T388_S011), SCXML_PASS, var1_is_3),
                                             ROW_IF(ENTERING(T388_S012), SCXML_FAIL, var1_is_3),
                                             ROW_IF(ENTERING(T388_S021), SCXML_FAIL, var1_is_3),
                                             ROW_IF(ENTERING(T388_S022), SCXML_FAIL, var1_is_3),
                                             END};
static hsm4c_transition_t const t388_s1[] = {AUTO(T388_S0_HD), END};
static hsm4c_transition_t const t388_s2[] = {AUTO(T388_S0_HS), END};

static hsm4c_state_t const t388[T388_NUM] = {
    [HSM4C_ROOT] = {.initial = T388_START},
    [T388_START] = {.transitions = t388_start},
    [T388_S0] = {.initial = T388_S01, .entry_fn = inc_var1, .transitions = t388_s0},
    [T388_S0_HS] = {.parent = T388_S0, .initial = T388_S02, .type = HSM4C_STATE_HISTORY},
    [T388_S0_HD] = {.parent = T388_S0, .initial = T388_S022, .type = HSM4C_STATE_HISTORY_DEEP},
    [T388_S01] = {.parent = T388_S0, .initial = T388_S011},
    [T388_S011] = {.parent = T388_S01, .entry_fn = raise_entering},
    [T388_S012] = {.parent = T388_S01, .entry_fn = raise_entering},
    [T388_S02] = {.parent = T388_S0, .initial = T388_S021},
    [T388_S021] = {.parent = T388_S02, .entry_fn = raise_entering},
    [T388_S022] = {.parent = T388_S02, .entry_fn = raise_entering},
    [T388_S1] = {.transitions = t388_s1},
    [T388_S2] = {.transitions = t388_s2},
};
hsm4c_chart_t const scxml_388 = HSM4C_CHART(t388);

/*
 * -------- 403a (3.13) --------
 * Transition selection: a matching transition in a child preempts the parent's, ties are
 * broken by document order, and the parent's transition is taken if the child has no enabled
 * one (s02's matching transition has a false guard).
 */

enum { T403A_S0 = 3, T403A_S01, T403A_S02, T403A_NUM };

static void t403a_s01_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  (void)hsm4c_raise(sm, EVENT1, NULL);
}
static void t403a_s02_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  (void)hsm4c_raise(sm, EVENT2, NULL);
}

static hsm4c_transition_t const t403a_s0[] = {ROW(EVENT1, SCXML_FAIL), ROW(EVENT2, SCXML_PASS),
                                              END};
static hsm4c_transition_t const t403a_s01[] = {ROW(EVENT1, T403A_S02), ROW(EVENT1, SCXML_FAIL),
                                               END};
static hsm4c_transition_t const t403a_s02[] = {ROW(EVENT1, SCXML_FAIL),
                                               ROW_IF(EVENT2, SCXML_FAIL, is_false), END};

static hsm4c_state_t const t403a[T403A_NUM] = {
    [HSM4C_ROOT] = {.initial = T403A_S0},
    [T403A_S0] = {.initial = T403A_S01, .transitions = t403a_s0},
    [T403A_S01] = {.parent = T403A_S0, .entry_fn = t403a_s01_entry, .transitions = t403a_s01},
    [T403A_S02] = {.parent = T403A_S0, .entry_fn = t403a_s02_entry, .transitions = t403a_s02},
};
hsm4c_chart_t const scxml_403a = HSM4C_CHART(t403a);

/*
 * -------- 407 (3.13) --------
 * Exiting a state executes its onexit handler.
 */

enum { T407_S0 = 3, T407_S1, T407_NUM };

static hsm4c_transition_t const t407_s0[] = {AUTO(T407_S1), END};
static hsm4c_transition_t const t407_s1[] = {AUTO_IF(SCXML_PASS, var1_is_1), AUTO(SCXML_FAIL), END};

static hsm4c_state_t const t407[T407_NUM] = {
    [HSM4C_ROOT] = {.initial = T407_S0},
    [T407_S0] = {.exit_fn = inc_var1, .transitions = t407_s0},
    [T407_S1] = {.transitions = t407_s1},
};
hsm4c_chart_t const scxml_407 = HSM4C_CHART(t407);

/*
 * -------- 411 (3.13) --------
 * A state is added to the active states before its onentry handler runs: in s0's onentry s01
 * is not active yet, in s01's onentry it is.
 */

enum { T411_S0 = 3, T411_S01, T411_NUM };

static void t411_s0_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  if (hsm4c_is_active(sm, T411_S01)) {
    (void)hsm4c_raise(sm, EVENT1, NULL);
  }
}
static void t411_s01_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  if (hsm4c_is_active(sm, T411_S01)) {
    (void)hsm4c_raise(sm, EVENT2, NULL);
  }
}

static hsm4c_transition_t const t411_s0[] = {ROW(EVENT1, SCXML_FAIL), ROW(EVENT2, SCXML_PASS), END};

static hsm4c_state_t const t411[T411_NUM] = {
    [HSM4C_ROOT] = {.initial = T411_S0},
    [T411_S0] = {.initial = T411_S01, .entry_fn = t411_s0_entry, .transitions = t411_s0},
    [T411_S01] = {.parent = T411_S0, .entry_fn = t411_s01_entry},
};
hsm4c_chart_t const scxml_411 = HSM4C_CHART(t411);

/*
 * -------- 419 (3.13) --------
 * Eventless transitions take precedence over event-driven ones, internal or external. Run with
 * SCXML_EXTERNAL after init.
 */

enum { T419_S1 = 3, T419_NUM };

static void t419_s1_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  (void)hsm4c_raise(sm, INTERNAL, NULL);
}

static hsm4c_transition_t const t419_s1[] = {
    ROW(INTERNAL, SCXML_FAIL), ROW(SCXML_EXTERNAL, SCXML_FAIL), AUTO(SCXML_PASS), END};

static hsm4c_state_t const t419[T419_NUM] = {
    [HSM4C_ROOT] = {.initial = T419_S1},
    [T419_S1] = {.entry_fn = t419_s1_entry, .transitions = t419_s1},
};
hsm4c_chart_t const scxml_419 = HSM4C_CHART(t419);

/*
 * -------- 421 (3.13) --------
 * Internal events take priority over external ones, and internal events are processed until
 * one enables a transition. Run with SCXML_EXTERNAL after init.
 */

enum { T421_S1 = 3, T421_S11, T421_S12, T421_NUM };

static void t421_s1_entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  (void)state;
  (void)hsm4c_raise(sm, INTERNAL1, NULL);
  (void)hsm4c_raise(sm, INTERNAL2, NULL);
  (void)hsm4c_raise(sm, INTERNAL3, NULL);
  (void)hsm4c_raise(sm, INTERNAL4, NULL);
}

static hsm4c_transition_t const t421_s1[] = {ROW(SCXML_EXTERNAL, SCXML_FAIL), END};
static hsm4c_transition_t const t421_s11[] = {ROW(INTERNAL3, T421_S12), END};
static hsm4c_transition_t const t421_s12[] = {ROW(INTERNAL4, SCXML_PASS), END};

static hsm4c_state_t const t421[T421_NUM] = {
    [HSM4C_ROOT] = {.initial = T421_S1},
    [T421_S1] = {.initial = T421_S11, .entry_fn = t421_s1_entry, .transitions = t421_s1},
    [T421_S11] = {.parent = T421_S1, .transitions = t421_s11},
    [T421_S12] = {.parent = T421_S1, .transitions = t421_s12},
};
hsm4c_chart_t const scxml_421 = HSM4C_CHART(t421);

/*
 * -------- 503 (3.13) --------
 * A targetless transition does not exit and re-enter its source state. var1 counts exits of
 * s2, var2 the targetless transitions.
 */

enum { T503_S1 = 3, T503_S2, T503_S3, T503_NUM };

static hsm4c_transition_t const t503_s1[] = {AUTO(T503_S2), END};
static hsm4c_transition_t const t503_s2[] = {ROW_DO(FOO, HSM4C_NO_STATE, act_inc_var2),
                                             ROW_IF(BAR, T503_S3, var2_is_1), ROW(BAR, SCXML_FAIL),
                                             END};
static hsm4c_transition_t const t503_s3[] = {AUTO_IF(SCXML_PASS, var1_is_1), AUTO(SCXML_FAIL), END};

static hsm4c_state_t const t503[T503_NUM] = {
    [HSM4C_ROOT] = {.initial = T503_S1},
    [T503_S1] = {.entry_fn = raise_foo_bar, .transitions = t503_s1},
    [T503_S2] = {.exit_fn = inc_var1, .transitions = t503_s2},
    [T503_S3] = {.transitions = t503_s3},
};
hsm4c_chart_t const scxml_503 = HSM4C_CHART(t503);

/*
 * -------- 505 (3.13) --------
 * A local (SCXML type="internal") transition to a descendant does not exit its source, but
 * exits the source's active descendants. var1 counts exits of s1, var2 exits of s11, var3 the
 * transition on foo.
 */

enum { T505_S1 = 3, T505_S11, T505_S2, T505_S3, T505_NUM };

static hsm4c_transition_t const t505_s1[] = {
    {.event = FOO, .to = T505_S11, .action_fn = act_inc_var3, .type = HSM4C_TRANSITION_LOCAL},
    ROW_IF(BAR, T505_S2, var3_is_1),
    ROW(BAR, SCXML_FAIL),
    END};
static hsm4c_transition_t const t505_s2[] = {AUTO_IF(T505_S3, var1_is_1), AUTO(SCXML_FAIL), END};
static hsm4c_transition_t const t505_s3[] = {AUTO_IF(SCXML_PASS, var2_is_2), AUTO(SCXML_FAIL), END};

static hsm4c_state_t const t505[T505_NUM] = {
    [HSM4C_ROOT] = {.initial = T505_S1},
    [T505_S1] = {.initial = T505_S11,
                 .entry_fn = raise_foo_bar,
                 .exit_fn = inc_var1,
                 .transitions = t505_s1},
    [T505_S11] = {.parent = T505_S1, .exit_fn = inc_var2},
    [T505_S2] = {.transitions = t505_s2},
    [T505_S3] = {.transitions = t505_s3},
};
hsm4c_chart_t const scxml_505 = HSM4C_CHART(t505);

/*
 * -------- 506 (3.13) --------
 * A local (SCXML type="internal") transition whose target is not a proper descendant of its
 * source (here: the source itself) behaves like an external transition. var1 counts exits of
 * s2, var2 exits of s21, var3 the transition on foo.
 */

enum { T506_S1 = 3, T506_S2, T506_S21, T506_S3, T506_S4, T506_NUM };

static hsm4c_transition_t const t506_s1[] = {AUTO(T506_S2), END};
static hsm4c_transition_t const t506_s2[] = {
    {.event = FOO, .to = T506_S2, .action_fn = act_inc_var3, .type = HSM4C_TRANSITION_LOCAL},
    ROW_IF(BAR, T506_S3, var3_is_1),
    ROW(BAR, SCXML_FAIL),
    END};
static hsm4c_transition_t const t506_s3[] = {AUTO_IF(T506_S4, var1_is_2), AUTO(SCXML_FAIL), END};
static hsm4c_transition_t const t506_s4[] = {AUTO_IF(SCXML_PASS, var2_is_2), AUTO(SCXML_FAIL), END};

static hsm4c_state_t const t506[T506_NUM] = {
    [HSM4C_ROOT] = {.initial = T506_S1},
    [T506_S1] = {.entry_fn = raise_foo_bar, .transitions = t506_s1},
    [T506_S2] = {.initial = T506_S21, .exit_fn = inc_var1, .transitions = t506_s2},
    [T506_S21] = {.parent = T506_S2, .exit_fn = inc_var2},
    [T506_S3] = {.transitions = t506_s3},
    [T506_S4] = {.transitions = t506_s4},
};
hsm4c_chart_t const scxml_506 = HSM4C_CHART(t506);

/*
 * -------- 580 (3.10) --------
 * A history state never becomes part of the active configuration. Adapted without the
 * <parallel> wrapper of the original: s1 is entered through its history once by default and
 * once with stored history, checking every time that the history state is not active. var1
 * counts exits of s1.
 */

enum { T580_START = 3, T580_S1, T580_S1_H, T580_S11, T580_S12, T580_NUM };

static bool t580_history_active(hsm4c_t const *sm, hsm4c_event_t const *e) {
  (void)e;
  return hsm4c_is_active(sm, T580_S1_H);
}

static hsm4c_transition_t const t580_start[] = {AUTO(T580_S1_H), END};
static hsm4c_transition_t const t580_s1[] = {AUTO_IF(SCXML_FAIL, t580_history_active),
                                             AUTO_IF(T580_S1_H, var1_is_0),
                                             AUTO_IF(SCXML_PASS, var1_is_1), END};
static hsm4c_transition_t const t580_s11[] = {AUTO_IF(SCXML_FAIL, t580_history_active),
                                              AUTO(T580_S12), END};

static hsm4c_state_t const t580[T580_NUM] = {
    [HSM4C_ROOT] = {.initial = T580_START},
    [T580_START] = {.transitions = t580_start},
    [T580_S1] = {.exit_fn = inc_var1, .transitions = t580_s1},
    [T580_S1_H] = {.parent = T580_S1, .initial = T580_S11, .type = HSM4C_STATE_HISTORY},
    [T580_S11] = {.parent = T580_S1, .transitions = t580_s11},
    [T580_S12] = {.parent = T580_S1},
};
hsm4c_chart_t const scxml_580 = HSM4C_CHART(t580);
