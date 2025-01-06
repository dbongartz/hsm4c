#include "hsm4c.h"

#include <stdio.h>

void entry_fn(void *ctx, hsm4c_state_t const *s) {
  (void)ctx;
#if HSM4C_CONFIG_STATE_NAME
  printf("entry: %s\n", s->name);
#else
  printf("entry: %p\n", (void *)s);
#endif
}

int main(void) {
  enum my_states_e {
    SROOT = HSM4C_STATE_ID_RESERVED,
    S0 = 1,
    S01,
    NUM_STATES,
  };

#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  static hsm4c_transition_t const transitions[] = {
      {
          .source = S0,
          .target = S0,
          .trigger.normal.id = 0,
      },
      {
          .source = S0,
          .target = S0,
          .trigger.variant = HSM4C_TRIGGER_COMPLETION_EVENT,
      },
  };
#endif

#if !HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  static hsm4c_transition_t const s0t[] = {
      {
          .target = S0,
          .trigger.normal.id = 0,
      },
      {
          .target = S0,
          .trigger.variant = HSM4C_TRIGGER_COMPLETION_EVENT,
      },
  };
#endif

  static hsm4c_state_t const states[NUM_STATES] = {
      [HSM4C_STATE_ID_RESERVED] = HSM4C_STATE_ROOT(S0),
      [S0] =
          {
#if HSM4C_CONFIG_STATE_NAME
              .name = "S0",
#endif
              .compound.initial = S01,
#if HSM4C_CONFIG_ENTRY_FN
              .compound.entry_fn = entry_fn,
#endif
#if !HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
              .compound.transitions = s0t,
              .compound.transitions_num = ARRAY_SIZE(s0t),
#endif
          },
      [S01] =
          {
#if HSM4C_CONFIG_STATE_NAME
              .name = "S01",
#endif
              .parent = S0,
#if HSM4C_CONFIG_ENTRY_FN
              .compound.entry_fn = entry_fn,
#endif
          },
  };
  static hsm4c_state_rt_t states_rt[NUM_STATES];

  static hsm4c_cfg_t const sm_cfg = {
      .states = states,
      .states_rt = states_rt,
      .num_states = ARRAY_SIZE(states),
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
      .transitions = transitions,
      .transitions_num = ARRAY_SIZE(transitions),
#endif
      .ctx = NULL,
  };

  hsm4c_t sm;

  printf("hsm4c_t: %zu, hsm4c_cfg_t: %zu, hsm4c_state_t: %zu, hsm4c_state_rt_t: %zu, "
         "hsm4c_transition_t: %zu, hsm4c_trigger_t: %zu\n",
         sizeof(hsm4c_t), sizeof(hsm4c_cfg_t), sizeof(hsm4c_state_t), sizeof(hsm4c_state_rt_t),
         sizeof(hsm4c_transition_t), sizeof(hsm4c_trigger_t));

  hsm4c_init(&sm, &sm_cfg);
  hsm4c_print_states(&sm);

  hsm4c_result_e result;
  do {
    result = hsm4c_run2completion(&sm, (hsm4c_trigger_t){.normal.id = 0});
  } while (result == HSM4C_DEFERED);

  hsm4c_print_states(&sm);
}
