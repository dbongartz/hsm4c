#include "test_chart.h"

static hsm4c_transition_t const transitions_a[] = {
    {EV_1, B, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_2, BB, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_7, B_H, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_13, A, t_guard, t_action, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_b[] = {
    {EV_1, A, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_3, A_H, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_4, A_H, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_5, A_DH, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aa[] = {
    {EV_3, AB, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_4, B, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_6, A_CHOICE, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_9, AAB, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_10, AAB, t_guard, t_action, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_ab[] = {
    {EV_3, B, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aaa[] = {
    {EV_4, AAB, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_8, AAA, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_11, AAA, t_guard, t_action, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aab[] = {
    {EV_12, AA, t_guard, t_action, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_a_choice[] = {
    {EV_NO_EVENT, B, t_choice_A, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_NO_EVENT, C, t_choice_B, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_state_t const states[NUM_STATES] = {
    [ROOT] =
        {
            .name = "ROOT",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .initial = A,
        },
    [A] =
        {
            .name = "A",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = ROOT,
            .initial = AA,
            .transitions = transitions_a,
        },
    [B] =
        {
            .name = "B",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = ROOT,
            .initial = BA,
            .transitions = transitions_b,
        },
    [C] =
        {
            .name = "C",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = ROOT,
        },
    [AA] =
        {
            .name = "AA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = A,
            .initial = AAA,
            .transitions = transitions_aa,
        },
    [AB] =
        {
            .name = "AB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = A,
            .transitions = transitions_ab,
        },
    [AC] =
        {
            .name = "AC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = A,
        },
    [BA] =
        {
            .name = "BA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = B,
        },
    [BB] =
        {
            .name = "BB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = B,
        },
    [BC] =
        {
            .name = "BC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = B,
        },
    [AAA] =
        {
            .name = "AAA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = AA,
            .transitions = transitions_aaa,
        },
    [AAB] =
        {
            .name = "AAB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = AA,
            .transitions = transitions_aab,
        },
    [A_H] =
        {
            .name = "A_H",
            .parent = A,
            .initial = AB,
            .type = HSM4C_STATE_HISTORY,
        },
    [B_H] =
        {
            .name = "B_H",
            .parent = B,
            .initial = BC,
            .type = HSM4C_STATE_HISTORY,
        },
    [A_DH] =
        {
            .name = "A_DH",
            .parent = A,
            .initial = AC,
            .type = HSM4C_STATE_HISTORY_DEEP,
        },
    [A_CHOICE] =
        {
            .name = "A_CHOICE",
            .parent = A,
            .type = HSM4C_STATE_CHOICE,
            .transitions = transitions_a_choice,
        },
};

static hsm4c_transition_t const transitions_xroot[] = {
    {EV_5, XQ, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_xp[] = {
    {EV_1, XQ, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_xpa[] = {
    {EV_NO_EVENT, XR, x_auto, NULL, HSM4C_TRANSITION_EXTERNAL},
    {EV_1, XPB, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_2, XQ, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    {EV_3, HSM4C_NO_STATE, t_guard, t_action, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};

#define XSTATE(n, ...)                                                                             \
  { .name = n, .entry_fn = s_entry, .exit_fn = s_exit, .run_fn = s_run, __VA_ARGS__ }
static hsm4c_state_t const xstates[NUM_XSTATES] = {
    [XROOT] = XSTATE("XROOT", .initial = XP, .transitions = transitions_xroot),
    [XP] = XSTATE("XP", .parent = XROOT, .initial = XPA, .transitions = transitions_xp),
    [XPA] = XSTATE("XPA", .parent = XP, .transitions = transitions_xpa),
    [XPB] = XSTATE("XPB", .parent = XP),
    [XQ] = XSTATE("XQ", .parent = XROOT),
    [XR] = XSTATE("XR", .parent = XROOT),
};

hsm4c_chart_t const chart = HSM4C_CHART(states);
hsm4c_chart_t const xchart = HSM4C_CHART(xstates);
