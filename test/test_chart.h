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

extern hsm4c_state_t states[NUM_STATES];

/*
 * Second chart for SCXML semantics:
 *
 * XROOT (initial XP, table: XP -> XQ on EV_1, XPA -> XPB on EV_1, XROOT -> XQ on EV_5)
 * +- XP (initial XPA)
 * |  +- XPA (table: XPA -> XR automatic [x_auto], XPA -> XQ on EV_2, XPA -> (none) on EV_3)
 * |  +- XPB
 * +- XQ
 * +- XR
 */
enum xstates { XROOT, XP, XPA, XPB, XQ, XR, NUM_XSTATES };

extern hsm4c_state_t xstates[NUM_XSTATES];

/** \brief Map configs and reset all states. */
void chart_reset(void);

/* hsm4c_state_t and transition functions. Implemented by the test (fakes). */
void s_entry(hsm4c_state_t const *s);
void s_exit(hsm4c_state_t const *s);
hsm4c_state_t *s_run(hsm4c_state_t const *s, hsm4c_event_id_t event);
bool t_guard(hsm4c_state_t const *root);
void t_action(hsm4c_state_t const *root);
bool t_choice_A(hsm4c_state_t const *root);
bool t_choice_B(hsm4c_state_t const *root);
bool x_auto(hsm4c_state_t const *root);

#ifdef __cplusplus
}
#endif
