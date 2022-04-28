#include "hsm4c.h"

#include <stdio.h>

void dispatch(state_m_t *statem, int event) {
  printf("Dispatching event %d...\n", event);

  const state_t *s = statem->state;

  for (size_t i = 0; i < s->tran_list_size; ++i) {
    const tran_t *tran = &s->tran_list[i];

    if (tran->event_n == event) {
      printf("Event %d found in state %p\n", event, s);

      if (tran->guard_fn && !tran->guard_fn(statem->data))
        return;

      if (s->exit_fn)
        s->exit_fn(statem->data);

      if (tran->action_fn)
        tran->action_fn(statem->data);

      s = tran->target_state;
      statem->state = s;

      printf("New state %p.\n", s);

      if (s->entry_fn)
        s->entry_fn(statem->data);

      return;
    }
  }

  printf("Event %d not handled\n", event);
}
































/*

  printf("Event %d not found in state %p\n", event, s);
  printf("Check parents...\n");

  const state_t *found = NULL;

  s = s->parent_s;
  while(s) {
    for (size_t i = 0; i < s->tran_list_size; ++i) {
      const tran_t *tran = &s->tran_list[i];
      if (tran->event_n == event) {
        found = s;
        break;
      }
    }
    if(found)
      break;

    s = s->parent_s;
  }

  if(found) {
    printf("Found parent %p\n", found);
    s = statem->state;
    while(s != found) {
      if(s->exit_fn)
        s->exit_fn(statem->data);
      s = s->parent_s;
    }
    statem->state = s;
    dispatch(statem, event);
    return;
  }

  // Now search all parents if someone can handle the event and remember it
  // Call all exits on the way to that parent
  // Switch to that state and dispatch again.
*/