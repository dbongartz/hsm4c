/**
 * \brief Implementation of statechart driver
 * \file
 *
 * (C) 2023 David Bongartz
 * MIT License
 */

#include "hsm4c.h"

#include <stdbool.h>
#include <stddef.h>

/* -------- Private -------- */

/** \brief Find common ancestor of two states. Must be same tree. */
static State *fca(State const *const left, State const *const right) {
  for (State const *_left = left; _left->parent != NULL; _left = _left->parent) {
    for (State const *_right = right; _right->parent != NULL; _right = _right->parent) {
      if (_left->parent == _right->parent) {
        return _left->parent;
      }
    }
  }
  return NULL;
}

/** \brief Walk up a branch and call exit_fn(). end_ancestor MUST be a valid ancestor or NULL. */
static void walk_up_exit(State const *const root, State const *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->parent) {
    if (start->exit_fn) {
      start->exit_fn(start);
    }
  }
}

/** \brief Walk up a branch and set active state to match the branch. end_ancestor MUST be a valid
 * ancestor or NULL. */
static void walk_up_set_active_state(State *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->parent) {
    start->parent->_active = start;
  }
}

/** \brief Walk down a branch and call entry_fn(). end_child MUST be a valid child or NULL. */
static State *walk_down_entry(State const *const root, State *start, State const *end_child) {
  State *last_node = start;
  for (State *s = start; s != end_child && s != NULL; s = s->_active) {
    if (s->entry_fn) {
      s->entry_fn(s);
    }
    last_node = s;
  }
  return last_node;
}

/** \brief Walk down a branch and set initial state to active if present. */
static void walk_down_init(State *start) {
  for (; start != NULL; start = start->_active) {
    start->_active = start->initial;
  }
}

/** \brief Finds current active leaf in a branch. */
static State *find_leaf(State const *start) {
  State const *leaf;
  for (leaf = start; leaf->_active != NULL; leaf = leaf->_active) {
  }
  return (State *)leaf;
}

/** \brief Finds root of statechart */
static State *find_root(State const *start) {
  State const *root = NULL;
  for (root = start; root->parent != NULL; root = root->parent) {
  }
  return (State *)root;
}

/** \brief Finds valid (matching or automatic) transition in active branch. */
static Transition const *find_transition(State const *const root, Transition const transitions[],
                                         EventType event) {
  for (Transition const *t = transitions; t->from != NULL;
       ++t) { // Outer loop can be removed if transitions live in State
    if (t->event == event || t->event == SC_NO_EVENT) {
      // Search from leaf to root.
      for (State const *s = root->_active; s != NULL; s = s->parent) {
        if (t->from == s) {
          if (!t->guard_fn || (t->guard_fn && t->guard_fn(root))) {
            return t;
          }
        }
      }
    }
  }

  return NULL;
}

/** \brief run active state, return if a new state got returned, NULL otherwise. */
static State *run_state(State const *const root, State *s, EventType e) {
  if (s->run_fn) {
    State *target_state = s->run_fn(s, e);
    if (target_state && target_state != root->_active) {
      return target_state;
    }
  }
  return NULL;
}

/** \brief See run_state. Do this for all states in a branch.  */
static State *ancestors_run(State const *root, EventType event) {
  for (State *s = root->_active; s != NULL; s = s->parent) {
    State *requested_state = run_state(root, s, event);
    if (requested_state) {
      return requested_state;
    }
  }
  return NULL;
}

/** \brief Depending on active state type, return target state. */
static State *get_target_state_from_type(Transition const *const t) {
  State *target_state = NULL;
  switch (t->to->type) {
  case SC_TYPE_NORMAL:
  case SC_TYPE_CHOICE:
    target_state = t->to;
    break;
  case SC_TYPE_HISTORY:
    if (!t->to->parent->_active) {
      if (t->to->initial) {
        target_state = t->to->initial;
      } else {
        target_state = t->to->parent->initial;
      }
    } else {
      target_state = t->to->parent->_active;
    }
    break;
  case SC_TYPE_HISTORY_DEEP:
    target_state = find_leaf(t->to->parent);
    if (target_state == t->to->parent) {
      target_state = t->to->initial;
    }
    break;
  case SC_TYPE_ROOT:
    target_state = NULL;
    break;
  }
  return target_state;
}

/* -------- Public -------- */

State const *sc_init(State *root, Transition const transitions[]) {
  walk_down_init(root);
  root->_active = walk_down_entry(root, root, NULL);
  root->_transitions = transitions;
  return root->_active;
}

void sc_reset_state(State *state) {
  state->_active = NULL;
  state->_transitions = NULL;
}

State const *sc_run(State *root, EventType event) {

  Transition const *t = find_transition(root, root->_transitions, event);

  if (!t) {
    State *requested_state = ancestors_run(root, event);
    if (requested_state) {
      t = &(Transition const){.from = root->_active, .to = requested_state};
    }
  }

  while (t) {
    // Handle StateType
    State *target_state = get_target_state_from_type(t);

    // Find common ancestor of active leaf and target
    State *ca = fca(t->from, target_state);

    State *leaf = NULL;

    leaf = find_leaf(t->from);

    // Set target branch active states to reach target
    walk_up_set_active_state(target_state, ca);

    // Exit all states on the active branch until ancestor
    if (t->type == SC_TTYPE_LOCAL) {
      ca = ca->_active;
    }
    walk_up_exit(root, leaf, ca);

    // Transition
    if (t->transition_fn)
      t->transition_fn(root);

    // Entry target branch incl. target
    walk_down_entry(root, ca->_active, target_state->_active);

    // We might have not initialized this state yet
    walk_down_init(target_state);

    // Walk down until leaf
    leaf = walk_down_entry(root, target_state->_active, NULL);
    root->_active = leaf ? leaf : target_state;
    t = NULL;

    // Check transitions of current state with no event
    t = find_transition(root, root->_transitions, SC_NO_EVENT);

    // Run all "run" functions including parents, continue change if requested
    if (!t) {
      State *requested_state = ancestors_run(root, event);
      if (requested_state) {
        t = &(Transition const){.from = root->_active, .to = requested_state};
      }
    }
  }

  return root->_active;
}
