#include "hsm4c.h"
#include <stdio.h>
#include <assert.h>

void dispatch(state_machine_t *statem, int event) {
  assert(statem);

  const state_t *s = statem->state;

  for (size_t i = 0; i < s->tran_list_size; ++i) {
    const tran_t *t = &s->tran_list[i];

    if (t->event == event) {
      printf("sm: Try handling event %d in %s at transition %d\n",
          event, s->name, (int)i);

      if (t->guard_fn && !t->guard_fn(statem->data)) {
        continue;
      }

      if (s->exit_fn)
        s->exit_fn(statem->data);

      if (t->action_fn)
        t->action_fn(statem->data);

      statem->state = t->target_state;

      printf("sm: New state %s\n", statem->state->name);

      if (statem->state->entry_fn)
        statem->state->entry_fn(statem->data);

      return;
    }
  }

  printf("sm: Event %d not handled\n", event);
}