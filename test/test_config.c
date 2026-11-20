/*
 * Smoke test that works with every configuration (see "Configuration" in hsm4c.h), e.g. with
 * names, run functions, guards and actions compiled out. Uses designated initializers only.
 *
 * ROOT (initial A)
 * +- A (B on EV_GO, raises EV_BACK on entry of B)
 * +- B (A on EV_BACK)
 */

#include <hsm4c/hsm4c.h>

#include <stdio.h>

enum { ROOT, A, B, NUM_STATES };
enum { EV_GO = 1, EV_BACK };

static int entries[NUM_STATES];

static void entry(hsm4c_t *sm, hsm4c_state_id_t state) {
  ++entries[state];
  if (state == B) {
    (void)hsm4c_raise(sm, EV_BACK, NULL);
  }
}

static hsm4c_transition_t const a_transitions[] = {{.event = EV_GO, .to = B},
                                                   HSM4C_TRANSITIONS_END};
static hsm4c_transition_t const b_transitions[] = {{.event = EV_BACK, .to = A},
                                                   HSM4C_TRANSITIONS_END};

static hsm4c_state_t const states[NUM_STATES] = {
    [ROOT] = {.initial = A},
    [A] = {.entry_fn = entry, .transitions = a_transitions},
    [B] = {.entry_fn = entry, .transitions = b_transitions},
};

static hsm4c_chart_t const chart = HSM4C_CHART(states);

#define EXPECT(cond)                                                                               \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      (void)fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #cond);                     \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

int main(void) {
  hsm4c_t sm;
  hsm4c_state_id_t active[NUM_STATES];

  EXPECT(hsm4c_validate(&chart, NULL) == HSM4C_OK);
  EXPECT(hsm4c_init(&sm, &chart, active, NULL) == HSM4C_OK);
  EXPECT(hsm4c_current(&sm) == A);

  // A -> B, entry of B raises EV_BACK: B -> A in the same call
  EXPECT(hsm4c_run(&sm, EV_GO, NULL) == HSM4C_OK);
  EXPECT(hsm4c_current(&sm) == A);
  EXPECT(entries[A] == 2 && entries[B] == 1);
  EXPECT(hsm4c_is_active(&sm, ROOT) && !hsm4c_is_active(&sm, B));

  (void)printf("config test passed: sizeof(hsm4c_t) %u, hsm4c_state_t %u, hsm4c_transition_t %u\n",
               (unsigned)sizeof(hsm4c_t), (unsigned)sizeof(hsm4c_state_t),
               (unsigned)sizeof(hsm4c_transition_t));
  return 0;
}
