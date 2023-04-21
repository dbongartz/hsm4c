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
  for (State const *_left = left; _left->config->parent != NULL; _left = _left->config->parent) {
    for (State const *_right = right; _right->config->parent != NULL;
         _right = _right->config->parent) {
      if (_left->config->parent == _right->config->parent) {
        return _left->config->parent;
      }
    }
  }
  return NULL;
}

/** \brief Walk up a branch and call exit_fn(). end_ancestor MUST be a valid ancestor or NULL. */
static void walk_up_exit(State const *const root, State const *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    if (start->config->exit_fn) {
      start->config->exit_fn(start);
    }
  }
}

/** \brief Walk up a branch and set active state to match the branch. end_ancestor MUST be a valid
 * ancestor or NULL. */
static void walk_up_set_active_state(State *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    start->config->parent->_active = start;
  }
}

/** \brief Walk down a branch and call entry_fn(). end_child MUST be a valid child or NULL. */
static State *walk_down_entry(State const *const root, State *start, State const *end_child) {
  State *last_node = start;
  for (State *s = start; s != end_child && s != NULL; s = s->_active) {
    if (s->config->entry_fn) {
      s->config->entry_fn(s);
    }
    last_node = s;
  }
  return last_node;
}

/** \brief Walk down a branch and set initial state to active if present. */
static void walk_down_init(State *start) {
  for (; start != NULL; start = start->_active) {
    start->_active = start->config->initial;
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
  for (root = start; root->config->parent != NULL; root = root->config->parent) {
  }
  return (State *)root;
}

/** \brief Finds valid (matching or automatic) transition in active branch. */
static Transition const *find_transition(State const *const root, EventType event) {
  Transition const *transitions = root->config->transitions;

  for (State const *s = root->_active; s != NULL; s = s->config->parent) {
    if (s->config->transitions) {
      transitions = s->config->transitions;
    }
    for (Transition const *t = transitions; t->from != NULL; ++t) {
      if (t->event == event || t->event == SC_NO_EVENT) {
        if (transitions != root->config->transitions) {
          if (!t->guard_fn || (t->guard_fn && t->guard_fn(root))) {
            return t;
          }
        } else {
          // Search from current state to root. TODO: Only needed when table is not sourced by state
          for (State const *parent = s; parent != NULL; parent = parent->config->parent) {
            if (t->from == parent) {
              if (!t->guard_fn || (t->guard_fn && t->guard_fn(root))) {
                return t;
              }
            }
          }
        }
      }
    }
  }

  return NULL;
}

/** \brief run active state, return if a new state got returned, NULL otherwise. */
static State *run_state(State const *const root, State *s, EventType e) {
  if (s->config->run_fn) {
    State *target_state = s->config->run_fn(s, e);
    if (target_state && target_state != root->_active) {
      return target_state;
    }
  }
  return NULL;
}

/** \brief See run_state. Do this for all states in a branch.  */
static State *ancestors_run(State const *root, EventType event) {
  for (State *s = root->_active; s != NULL; s = s->config->parent) {
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
  switch (t->to->config->type) {
  case SC_TYPE_NORMAL:
  case SC_TYPE_CHOICE:
    target_state = t->to;
    break;
  case SC_TYPE_HISTORY:
    if (!t->to->config->parent->_active) {
      if (t->to->config->initial) {
        target_state = t->to->config->initial;
      } else {
        target_state = t->to->config->parent->config->initial;
      }
    } else {
      target_state = t->to->config->parent->_active;
    }
    break;
  case SC_TYPE_HISTORY_DEEP:
    target_state = find_leaf(t->to->config->parent);
    if (target_state == t->to->config->parent) {
      target_state = t->to->config->initial;
    }
    break;
  case SC_TYPE_ROOT:
    target_state = NULL;
    break;
  }
  return target_state;
}

/* -------- Public -------- */

State const *sc_init(State *root) {
  walk_down_init(root);
  root->_active = walk_down_entry(root, root, NULL);
  return root->_active;
}

void sc_map_stateconfig_to_states(size_t num_states, State states[num_states],
                                  StateConfig const statecfgs[num_states]) {
  for (int i = 0; i < num_states; ++i) {
    states[i].config = &statecfgs[i];
  }
}

void sc_reset_state(State *state) {
  state->_active = NULL;
}

State const *sc_get_root(State const *s) { return find_root(s); }

State const *sc_run(State *root, EventType event) {

  Transition const *t = find_transition(root, event);

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
    t = find_transition(root, SC_NO_EVENT);

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
