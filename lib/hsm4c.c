#include "hsm4c.h"

#include <stdio.h>

void dispatch(state_m_t *statem, int event) {
  printf("Dispatching event %d...\n", event);

  const state_t *state = statem->state;

  for (size_t i = 0; i < state->tran_list_size; ++i) {
    const tran_t *tran = &state->tran_list[i];

    if (tran->event_n == event) {
      printf("Event %d found in state %p\n", event, state);

      if (tran->guard_fn && !tran->guard_fn(statem->data))
        return;

      if (state->exit_fn)
        state->exit_fn(statem->data);

      if (tran->action_fn)
        tran->action_fn(statem->data);

      printf("Old state %p.\n", state);

      state = tran->target_state;
      statem->state = state;

      printf("New state %p.\n", state);

      if (state->entry_fn)
        state->entry_fn(statem->data);

      return;
    }
  }
  printf("Event %d not found in state %p\n", event, state);
}

void test(state_m_t *statem) {
  dispatch(statem, 1);
  dispatch(statem, 2);
  dispatch(statem, 2);
  dispatch(statem, 4);
  dispatch(statem, 3);
}