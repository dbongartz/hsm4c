#include "test_chart.h"

hsm4c_state_t states[NUM_STATES];

static hsm4c_transition_t const transitions_a[] = {
    {&states[A], &states[B], EV_1, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[A], &states[BB], EV_2, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[A], &states[B_H], EV_7, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[A], &states[A], EV_13, t_action, t_guard, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_b[] = {
    {&states[B], &states[A], EV_1, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[B], &states[A_H], EV_3, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[B], &states[A_H], EV_4, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[B], &states[A_DH], EV_5, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aa[] = {
    {&states[AA], &states[AB], EV_3, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AA], &states[B], EV_4, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AA], &states[A_CHOICE], EV_6, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AA], &states[AAB], EV_9, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AA], &states[AAB], EV_10, t_action, t_guard, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_ab[] = {
    {&states[AB], &states[B], EV_3, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aaa[] = {
    {&states[AAA], &states[AAB], EV_4, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AAA], &states[AAA], EV_8, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&states[AAA], &states[AAA], EV_11, t_action, t_guard, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_aab[] = {
    {&states[AAB], &states[AA], EV_12, t_action, t_guard, HSM4C_TRANSITION_LOCAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_a_choice[] = {
    {&states[A_CHOICE], &states[B], EV_NO_EVENT, t_action, t_choice_A, HSM4C_TRANSITION_EXTERNAL},
    {&states[A_CHOICE], &states[C], EV_NO_EVENT, t_action, t_choice_B, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};

static hsm4c_state_config_t const statecfgs[NUM_STATES] = {
    [ROOT] =
        {
            .name = "ROOT",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .initial = &states[A],
            .type = HSM4C_STATE_ROOT,
        },
    [A] =
        {
            .name = "A",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
            .initial = &states[AA],
            .transitions = transitions_a,
        },
    [B] =
        {
            .name = "B",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
            .initial = &states[BA],
            .transitions = transitions_b,
        },
    [C] =
        {
            .name = "C",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[ROOT],
        },
    [AA] =
        {
            .name = "AA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
            .initial = &states[AAA],
            .transitions = transitions_aa,
        },
    [AB] =
        {
            .name = "AB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
            .transitions = transitions_ab,
        },
    [AC] =
        {
            .name = "AC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[A],
        },
    [BA] =
        {
            .name = "BA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [BB] =
        {
            .name = "BB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [BC] =
        {
            .name = "BC",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[B],
        },
    [AAA] =
        {
            .name = "AAA",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[AA],
            .transitions = transitions_aaa,
        },
    [AAB] =
        {
            .name = "AAB",
            .entry_fn = s_entry,
            .exit_fn = s_exit,
            .run_fn = s_run,
            .parent = &states[AA],
            .transitions = transitions_aab,
        },
    [A_H] =
        {
            .name = "A_H",
            .parent = &states[A],
            .initial = &states[AB],
            .type = HSM4C_STATE_HISTORY,
        },
    [B_H] =
        {
            .name = "B_H",
            .parent = &states[B],
            .initial = &states[BC],
            .type = HSM4C_STATE_HISTORY,
        },
    [A_DH] =
        {
            .name = "A_DH",
            .parent = &states[A],
            .initial = &states[AC],
            .type = HSM4C_STATE_HISTORY_DEEP,
        },
    [A_CHOICE] =
        {
            .name = "A_CHOICE",
            .parent = &states[A],
            .type = HSM4C_STATE_CHOICE,
            .transitions = transitions_a_choice,
        },
};

hsm4c_state_t xstates[NUM_XSTATES];

static hsm4c_transition_t const transitions_xroot[] = {
    {&xstates[XP], &xstates[XQ], EV_1, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&xstates[XPA], &xstates[XPB], EV_1, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&xstates[XROOT], &xstates[XQ], EV_5, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_transition_t const transitions_xpa[] = {
    {&xstates[XPA], &xstates[XR], EV_NO_EVENT, NULL, x_auto, HSM4C_TRANSITION_EXTERNAL},
    {&xstates[XPA], &xstates[XQ], EV_2, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    {&xstates[XPA], NULL, EV_3, t_action, t_guard, HSM4C_TRANSITION_EXTERNAL},
    HSM4C_TRANSITIONS_END,
};

#define XSTATE(n, ...)                                                                             \
  { .name = n, .entry_fn = s_entry, .exit_fn = s_exit, .run_fn = s_run, __VA_ARGS__ }
static hsm4c_state_config_t const xstatecfgs[NUM_XSTATES] = {
    [XROOT] = XSTATE("XROOT", .initial = &xstates[XP], .type = HSM4C_STATE_ROOT,
                     .transitions = transitions_xroot),
    [XP] = XSTATE("XP", .parent = &xstates[XROOT], .initial = &xstates[XPA]),
    [XPA] = XSTATE("XPA", .parent = &xstates[XP], .transitions = transitions_xpa),
    [XPB] = XSTATE("XPB", .parent = &xstates[XP]),
    [XQ] = XSTATE("XQ", .parent = &xstates[XROOT]),
    [XR] = XSTATE("XR", .parent = &xstates[XROOT]),
};

void chart_reset(void) {
  hsm4c_map_stateconfig_to_states(NUM_STATES, states, statecfgs);
  for (size_t i = 0; i < NUM_STATES; ++i) {
    hsm4c_reset_state(&states[i]);
  }
  hsm4c_map_stateconfig_to_states(NUM_XSTATES, xstates, xstatecfgs);
  for (size_t i = 0; i < NUM_XSTATES; ++i) {
    hsm4c_reset_state(&xstates[i]);
  }
}
