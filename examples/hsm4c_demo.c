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

static State my_states[NUM_STATES];

/* -------- State functions -------- */

static void entry(State const *s) { printf("  entry  %s\n", s->config->name); }

static void exit_(State const *s) { printf("  exit   %s\n", s->config->name); }

static State *run(State const *s, EventType e) {
  printf("  run    %s (event %d)\n", s->config->name, e);
  return NULL;
}

static State *branch_run(State const *s, EventType e) {
  printf("  run    %s (event %d) -> D\n", s->config->name, e);
  return &my_states[D];
}

/* -------- Transition functions -------- */

static void action(State const *root) { printf("  action (root %s)\n", root->config->name); }

static bool guard(State const *root) {
  printf("  guard  (root %s)\n", root->config->name);
  return true;
}

/** \brief False on the first call, true afterwards. */
static bool condition(State const *root) {
  static int counter = 0;
  printf("  condition (root %s): call %d\n", root->config->name, counter);
  return counter++ >= 1;
}

/* -------- Statechart -------- */

static Transition const my_transitions[] = {
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
    {.from = &my_states[GB], .to = &my_states[GA], .event = SC_NO_EVENT, .guard_fn = condition},
    {.from = &my_states[GA], .to = &my_states[A], .event = SC_NO_EVENT, .guard_fn = condition},
    SC_TRANSITIONS_END,
};

#define STATE(n, ...)                                                                              \
  { .name = n, .entry_fn = entry, .exit_fn = exit_, __VA_ARGS__ }

static StateConfig const my_statecfgs[NUM_STATES] = {
    [ROOT] = {.name = "ROOT",
              .initial = &my_states[A],
              .type = SC_TYPE_ROOT,
              .transitions = my_transitions},
    [A] = STATE("A", .parent = &my_states[ROOT], .initial = &my_states[C]),
    [A_H] = {.name = "A_H", .parent = &my_states[A], .type = SC_TYPE_HISTORY},
    [A_HD] = {.name = "A_HD",
              .parent = &my_states[A],
              .initial = &my_states[C],
              .type = SC_TYPE_HISTORY_DEEP},
    [BRANCH] = {.name = "BRANCH", .run_fn = branch_run, .parent = &my_states[A]},
    [B] = STATE("B", .run_fn = run, .parent = &my_states[ROOT]),
    [C] = STATE("C", .parent = &my_states[A]),
    [D] = STATE("D", .parent = &my_states[A], .initial = &my_states[E]),
    [D_H] = {.name = "D_H",
             .parent = &my_states[D],
             .initial = &my_states[E],
             .type = SC_TYPE_HISTORY},
    [E] = STATE("E", .parent = &my_states[D]),
    [F] = STATE("F", .run_fn = run, .parent = &my_states[D]),
    [G] = STATE("G", .parent = &my_states[ROOT], .initial = &my_states[GA]),
    [G_HD] = {.name = "G_HD",
              .parent = &my_states[G],
              .initial = &my_states[GB],
              .type = SC_TYPE_HISTORY_DEEP},
    [GA] = STATE("GA", .parent = &my_states[G]),
    [GB] = STATE("GB", .parent = &my_states[G]),
};

/* -------- Main -------- */

struct step {
  EventType event;
  char const *expected;
};

int main(void) {
  static struct step const steps[] = {
      {1, "B"},
      {SC_NO_EVENT, "B (no change)"},
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
      {SC_NO_EVENT, "C (no change)"},
  };

  printf("hsm4c demo\n");
  printf("sizeof(State): %zu, sizeof(Transition): %zu\n\n", sizeof(State), sizeof(Transition));

  State *my_sm = &my_states[ROOT];
  sc_map_stateconfig_to_states(NUM_STATES, my_states, my_statecfgs);

  printf("sc_init()\n");
  State const *current = sc_init(my_sm);
  printf("-> %s\n\n", current->config->name);

  for (size_t i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
    printf("sc_run(%d), expected: %s\n", steps[i].event, steps[i].expected);
    current = sc_run(my_sm, steps[i].event);
    printf("-> %s\n\n", current->config->name);
  }

  return 0;
}
