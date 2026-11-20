#pragma once
/* Test statechart. See test_hsm4c_statechart.puml */

#include <hsm4c/hsm4c.h>

#ifdef __cplusplus
extern "C" {
#endif

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
  NUM_STATES
};

enum events {
  EV_NO_EVENT = HSM4C_NO_EVENT,
  EV_1,
  EV_2,
  EV_3,
  EV_4,
  EV_5,
  EV_6,
  EV_7,
  EV_8,
  EV_9,
  EV_10,
  EV_11,
  EV_12,
  EV_13
};

extern hsm4c_chart_t const chart;

/*
 * Second chart for SCXML semantics:
 *
 * XROOT (initial XP; XQ on EV_5)
 * +- XP (initial XPA; XQ on EV_1)
 * |  +- XPA (XR automatic [x_auto], XPB on EV_1, XQ on EV_2, targetless on EV_3, XL1 on EV_6,
 * |         XPB on EV_7 / x_raise)
 * |  +- XPB (XR on EV_8)
 * +- XQ
 * +- XR
 * +- XL1 (XL2 automatic)
 * +- XL2 (XL1 automatic)
 */
enum xstates { XROOT, XP, XPA, XPB, XQ, XR, XL1, XL2, NUM_XSTATES };

extern hsm4c_chart_t const xchart;

/* State and transition functions. Implemented by the test (fakes). */
void s_entry(hsm4c_t *sm, hsm4c_state_id_t state);
void s_exit(hsm4c_t *sm, hsm4c_state_id_t state);
hsm4c_state_id_t s_run(hsm4c_t *sm, hsm4c_state_id_t state, hsm4c_event_t const *event);
bool t_guard(hsm4c_t const *sm, hsm4c_event_t const *event);
void t_action(hsm4c_t *sm, hsm4c_event_t const *event);
bool t_choice_A(hsm4c_t const *sm, hsm4c_event_t const *event);
bool t_choice_B(hsm4c_t const *sm, hsm4c_event_t const *event);
bool x_auto(hsm4c_t const *sm, hsm4c_event_t const *event);
void x_raise(hsm4c_t *sm, hsm4c_event_t const *event);

#ifdef __cplusplus
}
#endif
