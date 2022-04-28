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

// Find lowest common ancestor
static state_t *find_lcr(state_t *a, state_t *b) {
  state_t *entry_b = b;
  while (a) {
    while (b) {
      if (a == b) {
        return a;
      }
      b = b->parent;
    }
    b = entry_b;
    a = a->parent;
  }

  return NULL;
}

static state_t * find_leaf(state_t *s) {
  // Find leaf
  state_t *found = NULL;
  while (s) {
    found = s;
    s = s->child;
  }
  return found;
}

static state_t * find_root(state_t *s) {
  // Find leaf
  state_t *found = NULL;
  while (s) {
    found = s;
    s = s->parent;
  }
  return found;
}

typedef struct {
  state_t *handler;
  tran_t *trans;
} handler_trans_t;

static handler_trans_t find_handler_and_trans(state_t * state, int event) {

  while (state) {
    for (size_t i = 0; i < state->tran_list_size; ++i) {
      tran_t *trans = &state->tran_list[i];

      if (trans->event_n == event) {
        if(trans->guard_fn && trans->guard_fn(&event)) {
          return (handler_trans_t) { .handler = state, .trans = trans};
        }
      }
    }
    state = state->parent;
  }

  return (handler_trans_t) { NULL, NULL};
}

static void print_branch(state_t * root) {
  printf("Root state: %s with children: ", root->name);
  state_t *s = s->child;
  while (s) {
    printf("%s ", s->name);
    s = s->child;
  }
  putchar('\n');
}

bool dispatch_hsm(state_t *s, int event) {
  printf("Dispatching event %d...\n", event);

  // Walk down all childs until leaf
  // Check if event can be handled there
  // Get handling child and target state

  // (Abstraction)
  // Find all nodes walking up from handling state (exit) and walking down
  // (entry) to reach target state Set state to init when no history and exit()
  // Set state to next child walking down and entry()

  // (Concrete algorithm)
  // Find common state of handling child and target state (tree)
  // Walk up self parents to common node and call exits
  // Set each parents child state to its initial state (no history)

  // Call transition action

  // Walk up target parents to common node and set each to the child coming
  // from. Walk down again and call entry()

  // Walk down target childs and call entry()

  state_t *walker = NULL;
  state_t *handler = NULL;
  tran_t *trans = NULL;
  state_t *target = NULL;
  state_t *leaf = NULL;
  state_t *lcr = NULL;

  leaf = find_leaf(s);

  // Walk up again trying to handle the event
  handler_trans_t handler_trans = find_handler_and_trans(leaf, event);

  handler = handler_trans.handler;
  trans = handler_trans.trans;

  if (handler && trans) {
    target = trans->target_state;

    printf("Handling event %d in state %s targeting %s\n", event, handler->name,
           target->name);

    if (handler != target) {

      // Find lowest common ancestor
      lcr = find_lcr(leaf, target);

      // Walk up from leaf, exit and reset
      walker = leaf;
      while (walker != lcr) {
        // Exit
        if (walker->exit_fn) {
          walker->exit_fn(walker->name);
        }
        // Reset
        walker->child = walker->initial;

        walker = walker->parent;
      }

      // Transition
      if (trans->action_fn) {
        trans->action_fn(&trans->event_n);
      }

      // Walk up from target, set child
      walker = target;
      while (walker != lcr) {
        if (walker->parent) {
          walker->parent->child = walker;
        }
        walker = walker->parent;
      }

      // Walk down from lcr till leaf and entry
      walker = lcr->child;
      while (walker) {
        if (walker->entry_fn) {
          walker->entry_fn(walker->name);
        }
        walker = walker->child;
      }
    } else {
      // Just transition and return
      if (trans->action_fn) {
        trans->action_fn(&trans->event_n);
      }
    }

    print_branch(find_root(s));

    return true;
  }

  printf("Event %d not handled\n", event);

  print_branch(find_root(s));

  return false;
}