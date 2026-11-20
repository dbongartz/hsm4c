/**
 * hsm4c demo
 *
 * ROOT
 * +- A (initial C)
 * |  +- A_H     shallow history
 * |  +- A_HD    deep history (initial C)
 * |  +- BRANCH  run function transitions to D
 * |  +- C
 * |  +- D (initial E)
 * |     +- D_H  shallow history (initial E)
 * |     +- E
 * |     +- F
 * +- B
 * +- G (initial GA)
 *    +- G_HD    deep history (initial GB)
 *    +- GA
 *    +- GB
 */

#include <hsm4c/hsm4c.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum my_states { ROOT, A, A_H, A_HD, BRANCH, B, C, D, D_H, E, F, G, G_HD, GA, GB, NUM_STATES };

static hsm4c_state_t my_states[NUM_STATES];

/* -------- State functions -------- */

static void entry(hsm4c_state_t const *s) { printf("  entry  %s\n", s->config->name); }

static void exit_(hsm4c_state_t const *s) { printf("  exit   %s\n", s->config->name); }

static hsm4c_state_t *run(hsm4c_state_t const *s, hsm4c_event_id_t e) {
  printf("  run    %s (event %d)\n", s->config->name, e);
  return NULL;
}

static hsm4c_state_t *branch_run(hsm4c_state_t const *s, hsm4c_event_id_t e) {
  printf("  run    %s (event %d) -> D\n", s->config->name, e);
  return &my_states[D];
}

/* -------- Transition functions -------- */

static void action(hsm4c_state_t const *root) {
  printf("  action (root %s)\n", root->config->name);
}

static bool guard(hsm4c_state_t const *root) {
  printf("  guard  (root %s)\n", root->config->name);
  return true;
}

/** \brief False on the first call, true afterwards. */
static bool condition(hsm4c_state_t const *root) {
  static int counter = 0;
  printf("  condition (root %s): call %d\n", root->config->name, counter);
  return counter++ >= 1;
}

/* -------- Statechart -------- */

static hsm4c_transition_t const my_transitions[] = {
    {.from = &my_states[A],
     .to = &my_states[B],
     .event = 1,
     .transition_fn = action,
     .guard_fn = guard},
    {.from = &my_states[B],
     .to = &my_states[A],
     .event = 2,
     .transition_fn = action,
     .guard_fn = guard},
    {.from = &my_states[B],
     .to = &my_states[A_H],
     .event = 3,
     .transition_fn = action,
     .guard_fn = guard},
    {.from = &my_states[C], .to = &my_states[D_H], .event = 4},
    {.from = &my_states[E], .to = &my_states[F], .event = 5},
    {.from = &my_states[B],
     .to = &my_states[A_HD],
     .event = 6,
     .transition_fn = action,
     .guard_fn = guard},
    {.from = &my_states[F], .to = &my_states[BRANCH], .event = 7},
    {.from = &my_states[E], .to = &my_states[G_HD], .event = 7},
    {.from = &my_states[GB], .to = &my_states[GA], .event = HSM4C_NO_EVENT, .guard_fn = condition},
    {.from = &my_states[GA], .to = &my_states[A], .event = HSM4C_NO_EVENT, .guard_fn = condition},
    HSM4C_TRANSITIONS_END,
};

#define STATE(n, ...)                                                                              \
  { .name = n, .entry_fn = entry, .exit_fn = exit_, __VA_ARGS__ }

static hsm4c_state_config_t const my_statecfgs[NUM_STATES] = {
    [ROOT] = {.name = "ROOT",
              .initial = &my_states[A],
              .type = HSM4C_STATE_ROOT,
              .transitions = my_transitions},
    [A] = STATE("A", .parent = &my_states[ROOT], .initial = &my_states[C]),
    [A_H] = {.name = "A_H", .parent = &my_states[A], .type = HSM4C_STATE_HISTORY},
    [A_HD] = {.name = "A_HD",
              .parent = &my_states[A],
              .initial = &my_states[C],
              .type = HSM4C_STATE_HISTORY_DEEP},
    [BRANCH] = {.name = "BRANCH", .run_fn = branch_run, .parent = &my_states[A]},
    [B] = STATE("B", .run_fn = run, .parent = &my_states[ROOT]),
    [C] = STATE("C", .parent = &my_states[A]),
    [D] = STATE("D", .parent = &my_states[A], .initial = &my_states[E]),
    [D_H] = {.name = "D_H",
             .parent = &my_states[D],
             .initial = &my_states[E],
             .type = HSM4C_STATE_HISTORY},
    [E] = STATE("E", .parent = &my_states[D]),
    [F] = STATE("F", .run_fn = run, .parent = &my_states[D]),
    [G] = STATE("G", .parent = &my_states[ROOT], .initial = &my_states[GA]),
    [G_HD] = {.name = "G_HD",
              .parent = &my_states[G],
              .initial = &my_states[GB],
              .type = HSM4C_STATE_HISTORY_DEEP},
    [GA] = STATE("GA", .parent = &my_states[G]),
    [GB] = STATE("GB", .parent = &my_states[G]),
};

/* -------- Main -------- */

struct step {
  hsm4c_event_id_t event;
  char const *expected;
};

int main(void) {
  static struct step const steps[] = {
      {1, "B"},
      {HSM4C_NO_EVENT, "B (no change)"},
      {2, "C (A initial)"},
      {4, "E (D_H without history -> its initial E)"},
      {1, "B"},
      {3, "E (A_H: history of A is D, D is reset)"},
      {5, "F"},
      {1, "B"},
      {3, "E (A_H: history of A, not of D)"},
      {5, "F"},
      {1, "B"},
      {6, "F (A_HD: deep history of A, including D)"},
      {7, "E (F -> BRANCH, run of BRANCH -> D)"},
      {7, "GB (G_HD without history -> its initial GB, condition false)"},
      {7, "C (condition true: GB -> GA -> A)"},
      {8, "C (no change, no transition for event 8)"},
      {HSM4C_NO_EVENT, "C (no change)"},
  };

  printf("hsm4c demo\n");
  printf("sizeof(hsm4c_state_t): %zu, sizeof(hsm4c_transition_t): %zu\n\n", sizeof(hsm4c_state_t),
         sizeof(hsm4c_transition_t));

  hsm4c_state_t *my_sm = &my_states[ROOT];
  hsm4c_map_stateconfig_to_states(NUM_STATES, my_states, my_statecfgs);

  printf("hsm4c_init()\n");
  hsm4c_state_t const *current = hsm4c_init(my_sm);
  printf("-> %s\n\n", current->config->name);

  for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    printf("hsm4c_run(%d), expected: %s\n", steps[i].event, steps[i].expected);
    current = hsm4c_run(my_sm, steps[i].event);
    printf("-> %s\n\n", current->config->name);
  }

  return 0;
}
