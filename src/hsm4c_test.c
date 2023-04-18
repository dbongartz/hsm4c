#include <stdio.h>

#include "hsm4c.h"

/* ========================== */
enum my_states { ROOT, A, A_H, A_HD, BRANCH, B, C, D, D_H, E, F, G, G_HD, GA, GB };
static State my_states[];

static void state_a_entry(State const *sm) { printf("%s\n", __func__); }
static void state_a_exit(State const *sm) { printf("%s\n", __func__); }
static State *state_branch_run(State const *sm, EventType e) {
  printf("%s\n", __func__);
  return &my_states[D];
};
static void state_b_entry(State const *sm) { printf("%s\n", __func__); }
static void state_b_exit(State const *sm) { printf("%s\n", __func__); }
static void state_c_entry(State const *sm) { printf("%s\n", __func__); }
static void state_c_exit(State const *sm) { printf("%s\n", __func__); }
static State *state_b_run(State const *sm, EventType e) {
  printf("%s\n", __func__);
  //   return &my_states[C];
  return NULL;
};
static void state_d_entry(State const *sm) { printf("%s\n", __func__); }
static void state_d_exit(State const *sm) { printf("%s\n", __func__); }
static void state_e_entry(State const *sm) { printf("%s\n", __func__); }
static void state_e_exit(State const *sm) { printf("%s\n", __func__); }
static void state_f_entry(State const *sm) { printf("%s\n", __func__); }
static void state_f_exit(State const *sm) { printf("%s\n", __func__); }
static State *state_f_run(State const *sm, EventType e) {
  printf("%s\n", __func__);
  //   return &my_states[B];
  return NULL;
};
static void state_g_entry(State const *sm) { printf("%s\n", __func__); }
static void state_g_exit(State const *sm) { printf("%s\n", __func__); }
static void state_ga_entry(State const *sm) { printf("%s\n", __func__); }
static void state_ga_exit(State const *sm) { printf("%s\n", __func__); }
static void state_gb_entry(State const *sm) { printf("%s\n", __func__); }
static void state_gb_exit(State const *sm) { printf("%s\n", __func__); }

static void tran_1(State const *sm) { printf("%s\n", __func__); }
static void tran_2(State const *sm) { printf("%s\n", __func__); }
static bool guard_1(State const *sm) {
  printf("%s\n", __func__);
  return true;
}
static bool guard_2(State const *sm) {
  printf("%s\n", __func__);
  return true;
}
static bool condition_1(State const *sm) {
  static int counter = 0;
  printf("%s\n", __func__);
  return counter++ >= 1 ? true : false;
}

static State my_states[] = {
    [ROOT] =
        {
            .name = "my_sm",
            .initial = &my_states[A],
            .type = SC_TYPE_ROOT,
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
            .type = SC_TYPE_HISTORY,
        },

    [A_HD] =
        {
            .name = "A_H*",
            .parent = &my_states[A],
            .initial = &my_states[C],
            .type = SC_TYPE_HISTORY_DEEP,
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
            .type = SC_TYPE_HISTORY,
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
            .type = SC_TYPE_HISTORY_DEEP,
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

static Transition const my_transitions[] = {
    {&my_states[A], &my_states[B], 1, tran_1, guard_1},
    {&my_states[B], &my_states[A], 2, tran_2, guard_2},
    {&my_states[B], &my_states[A_H], 3, tran_2, guard_2},
    {&my_states[C], &my_states[D_H], 4},
    {&my_states[E], &my_states[F], 5},
    {&my_states[B], &my_states[A_HD], 6, tran_2, guard_2},
    {&my_states[F], &my_states[BRANCH], 7},
    {&my_states[E], &my_states[G_HD], 7},
    {&my_states[GB], &my_states[GA], SC_NO_EVENT, .guard_fn = condition_1},
    {&my_states[GA], &my_states[A], SC_NO_EVENT, .guard_fn = condition_1},
    SC_TRANSITIONS_END,
};

int main(void) {
  State const *current = NULL;
  State *my_sm = &my_states[ROOT];
  current = sc_init(my_sm, my_transitions);
  current = sc_run(my_sm, 1);           // B
  current = sc_run(my_sm, SC_NO_EVENT); // No change
  current = sc_run(my_sm, 2);           // A->C
  current = sc_run(my_sm, 4);           // A->D->E
  current = sc_run(my_sm, 1);           // B
  current = sc_run(my_sm, 3);           // A->D->E
  current = sc_run(my_sm, 5);           // A->D->F
  current = sc_run(my_sm, 1);           // B
  current = sc_run(my_sm, 3);           // A->D->E (history of A, not of D)
  current = sc_run(my_sm, 5);           // A->D->F
  current = sc_run(my_sm, 1);           // B
  current = sc_run(my_sm, 6);           // A->D->F (deep history of A, including D...)
  current = sc_run(my_sm, 7);           // B (run action of F)
  current = sc_run(my_sm, 7);           // A->C (run action of B)
  current = sc_run(my_sm, 7);           // A->D->E (run action of BRANCH)
  current = sc_run(my_sm, 8);           // G->GB (deep histoy with initial of G)
  current = sc_run(my_sm, 8);           // No change (conditional)
  current = sc_run(my_sm, SC_NO_EVENT); // G->GA (conditional >= 1) -> A->C (automatic)
}
