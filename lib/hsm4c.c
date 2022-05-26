#include "hsm4c.h"

#include <stdio.h>

typedef struct {
  state_t *handler;
  const tran_t *trans;
} handler_trans_t;

static void print_branch(state_t *root) {
  printf("Root state: %s with children: ", root->_state->name);
  state_t *s = s->child;
  while (s) {
    printf("%s ", s->_state->name);
    s = s->child;
  }
  putchar('\n');
}

// Find lowest common ancestor
static state_t *find_lca(state_t *a, state_t *b) {
  state_t *entry_b = b;
  while (a) {
    while (b) {
      if (a == b) {
        return a;
      }
      b = b->_state->parent;
    }
    b = entry_b;
    a = a->_state->parent;
  }

  return NULL;
}

static state_t *find_leaf(state_t *s) {
  state_t *found = NULL;
  while (s) {
    found = s;
    s = s->child;
  }
  return found;
}

static state_t *find_root(state_t *s) {
  state_t *found = NULL;
  while (s) {
    found = s;
    s = s->_state->parent;
  }
  return found;
}

static handler_trans_t find_handler_and_trans(state_t *state, event_t event) {
  while (state) {
    const const_state_t *cs = state->_state;
    for (size_t i = 0; i < cs->tran_list_size; ++i) {
      const tran_t *trans = &cs->tran_list[i];

      if (trans->event_n == event) {
        if (trans->guard_fn && trans->guard_fn(&event)) {
          return (handler_trans_t){.handler = state, .trans = trans};
        }
      }
    }
    state = cs->parent;
  }

  return (handler_trans_t){NULL, NULL};
}

static void walk_up_and_exit(state_t *start, state_t *target) {
  state_t *walker = start;
  while (walker != target) {
    // Exit
    if (walker->_state->exit_fn) {
      walker->_state->exit_fn((void *)walker->_state->name);
    }
    // Reset
    walker->child = walker->_state->initial;

    walker = walker->_state->parent;
  }
}

static void walk_up_and_set_child(state_t *start, state_t *target) {
  state_t *walker = start;
  while (walker != target) {
    if (walker->_state->parent) {
      walker->_state->parent->child = walker;
    }
    walker = walker->_state->parent;
  }
}

static void walk_down_and_entry(state_t *start) {
  state_t *walker = start->child;
  while (walker) {
    if (walker->_state->entry_fn) {
      walker->_state->entry_fn((void *)walker->_state->name);
    }
    walker = walker->child;
  }
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
  const tran_t *trans = NULL;
  state_t *target = NULL;
  state_t *leaf = NULL;
  state_t *lca = NULL;

  leaf = find_leaf(s);

  // Walk up again trying to handle the event
  handler_trans_t handler_trans = find_handler_and_trans(leaf, event);

  handler = handler_trans.handler;
  trans = handler_trans.trans;

  if (handler && trans) {
    target = trans->target_state;

    printf("Handling event %d in state %s targeting %s\n", event,
           handler->_state->name, target->_state->name);

    // Find lowest common ancestor
    lca = find_lca(leaf, target);

    // Walk up from leaf, exit and reset
    walk_up_and_exit(leaf, lca);

    // Transition
    if (trans->action_fn) {
      trans->action_fn((void *)&trans->event_n);
    }

    // Walk up from target, set child
    walk_up_and_set_child(target, lca);

    // Walk down from lcr till leaf and entry
    walk_down_and_entry(lca);

    print_branch(find_root(s));

    return true;
  }

  printf("Event %d not handled\n", event);

  print_branch(find_root(s));

  return false;
}

state_t *get_state(state_t *sm) { return find_leaf(sm); }
state_t *get_statemachine(state_t *s) { return find_root(s); }