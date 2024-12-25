#include <stdio.h>

#include "hsm4c.h"

/* ========================== */
enum my_states { ROOT, A, A_H, A_HD, BRANCH, B, C, D, D_H, E, F, G, G_HD, GA, GB, _NUM_STATES };
static hsm4c_state_t my_states[];

static void state_a_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_a_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static hsm4c_state_t *state_branch_run(hsm4c_state_t const *sm, hsm4c_event_t e) {
  printf("%s\n", __func__);
  return &my_states[D];
};
static void state_b_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_b_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_c_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_c_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static hsm4c_state_t *state_b_run(hsm4c_state_t const *sm, hsm4c_event_t e) {
  printf("%s\n", __func__);
  //   return &my_states[C];
  return NULL;
};
static void state_d_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_d_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_e_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_e_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_f_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_f_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static hsm4c_state_t *state_f_run(hsm4c_state_t const *sm, hsm4c_event_t e) {
  printf("%s\n", __func__);
  //   return &my_states[B];
  return NULL;
};
static void state_g_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_g_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_ga_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_ga_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_gb_entry(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void state_gb_exit(hsm4c_state_t const *sm) { printf("%s\n", __func__); }

static void tran_1(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static void tran_2(hsm4c_state_t const *sm) { printf("%s\n", __func__); }
static bool guard_1(hsm4c_state_t const *sm) {
  printf("%s\n", __func__);
  return true;
}
static bool guard_2(hsm4c_state_t const *sm) {
  printf("%s\n", __func__);
  return true;
}
static bool condition_1(hsm4c_state_t const *sm) {
  static int counter = 0;
  printf("%s\n", __func__);
  return counter++ >= 1 ? true : false;
}
static hsm4c_state_t my_states[_NUM_STATES];
static hsm4c_transition_t const my_transitions[] = {
    {&my_states[A], &my_states[B], 1, tran_1, guard_1},
    {&my_states[B], &my_states[A], 2, tran_2, guard_2},
    {&my_states[B], &my_states[A_H], 3, tran_2, guard_2},
    {&my_states[C], &my_states[D_H], 4},
    {&my_states[E], &my_states[F], 5},
    {&my_states[B], &my_states[A_HD], 6, tran_2, guard_2},
    {&my_states[F], &my_states[BRANCH], 7},
    {&my_states[E], &my_states[G_HD], 7},
    {&my_states[GB], &my_states[GA], HSM4C_NO_EVENT, .guard_fn = condition_1},
    {&my_states[GA], &my_states[A], HSM4C_NO_EVENT, .guard_fn = condition_1},
    HSM4C_TRANSITIONS_END,
};
static hsm4c_state_config_t const my_statecfgs[_NUM_STATES] = {
    [ROOT] =
        {
            .name = "my_sm",
            .initial = &my_states[A],
            .type = HSM4C_TYPE_ROOT,
            .transitions = my_transitions,
        },
    [A] = {.name = "A",
           .entry_fn = state_a_entry,
           .exit_fn = state_a_exit,
           .parent = &my_states[ROOT],
           .initial = &my_states[C]},
    [BRANCH] =
        {
            .name = "BRANCH",
            .run_fn = state_branch_run,
            .parent = &my_states[A],
        },
    [A_H] =
        {
            .name = "A_H",
            .parent = &my_states[A],
            .type = HSM4C_TYPE_HISTORY,
        },

    [A_HD] =
        {
            .name = "A_H*",
            .parent = &my_states[A],
            .initial = &my_states[C],
            .type = HSM4C_TYPE_HISTORY_DEEP,
        },
    [B] =
        {
            .name = "B",
            .entry_fn = state_b_entry,
            .exit_fn = state_b_exit,
            .run_fn = state_b_run,
            .parent = &my_states[ROOT],
        },
    [C] =
        {
            .name = "C",
            .entry_fn = state_c_entry,
            .exit_fn = state_c_exit,
            .parent = &my_states[A],
        },
    [D] =
        {
            .name = "D",
            .entry_fn = state_d_entry,
            .exit_fn = state_d_exit,
            .parent = &my_states[A],
            .initial = &my_states[E],
        },
    [D_H] =
        {
            .name = "D_H",
            .parent = &my_states[D],
            .initial = &my_states[E],
            .type = HSM4C_TYPE_HISTORY,
        },
    [E] =
        {
            .name = "E",
            .entry_fn = state_e_entry,
            .exit_fn = state_e_exit,
            .parent = &my_states[D],
        },
    [F] =
        {
            .name = "F",
            .entry_fn = state_f_entry,
            .exit_fn = state_f_exit,
            .run_fn = state_f_run,
            .parent = &my_states[D],
        },
    [G] =
        {
            .name = "G",
            .entry_fn = state_g_entry,
            .exit_fn = state_g_exit,
            .parent = &my_states[ROOT],
            .initial = &my_states[GA],
        },
    [G_HD] =
        {
            .name = "G_HD",
            .parent = &my_states[G],
            .initial = &my_states[GB],
            .type = HSM4C_TYPE_HISTORY_DEEP,
        },
    [GA] =
        {
            .name = "GA",
            .entry_fn = state_ga_entry,
            .exit_fn = state_ga_exit,
            .parent = &my_states[G],
        },
    [GB] =
        {
            .name = "GA",
            .entry_fn = state_gb_entry,
            .exit_fn = state_gb_exit,
            .parent = &my_states[G],
        },
};

int main(void) {
  printf("hsm4c demo\n");
  printf("sizeof(hsm4c_state_t): %lu, sizeof(hsm4c_transition_t): %lu\n\n", sizeof(hsm4c_state_t),
         sizeof(hsm4c_transition_t));

  hsm4c_state_t const *current = NULL;
  hsm4c_state_t *my_sm = &my_states[ROOT];
  hsm4c_assign_stateconfigs_to_states(_NUM_STATES, my_states, my_statecfgs);
  current = hsm4c_init(my_sm);
  current = hsm4c_run(my_sm, 1);              // B
  current = hsm4c_run(my_sm, HSM4C_NO_EVENT); // No change
  current = hsm4c_run(my_sm, 2);              // A->C
  current = hsm4c_run(my_sm, 4);              // A->D->E
  current = hsm4c_run(my_sm, 1);              // B
  current = hsm4c_run(my_sm, 3);              // A->D->E
  current = hsm4c_run(my_sm, 5);              // A->D->F
  current = hsm4c_run(my_sm, 1);              // B
  current = hsm4c_run(my_sm, 3);              // A->D->E (history of A, not of D)
  current = hsm4c_run(my_sm, 5);              // A->D->F
  current = hsm4c_run(my_sm, 1);              // B
  current = hsm4c_run(my_sm, 6);              // A->D->F (deep history of A, including D...)
  current = hsm4c_run(my_sm, 7);              // B (run action of F)
  current = hsm4c_run(my_sm, 7);              // A->C (run action of B)
  current = hsm4c_run(my_sm, 7);              // A->D->E (run action of BRANCH)
  current = hsm4c_run(my_sm, 8);              // G->GB (deep histoy with initial of G)
  current = hsm4c_run(my_sm, 8);              // No change (conditional)
  current = hsm4c_run(my_sm, HSM4C_NO_EVENT); // G->GA (conditional >= 1) -> A->C (automatic)
}
