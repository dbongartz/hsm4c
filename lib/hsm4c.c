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

/** \brief Finds the common ancestor of two states. Both must be in the same tree. */
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

/** \brief Walks up a branch and calls exit_fn(). end_ancestor MUST be a valid ancestor or NULL. */
static void walk_up_exit(State const *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    if (start->config->exit_fn) {
      start->config->exit_fn(start);
    }
  }
}

/** \brief Walks up a branch and sets the active states to match the branch. end_ancestor MUST be
 * a valid ancestor or NULL. */
static void walk_up_set_active_state(State *start, State const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    start->config->parent->_active = start;
  }
}

/** \brief Walks down a branch from start to end_child and calls entry_fn(). */
static void walk_down_entry(State const * const start, State const * const end_child) {
  if (!end_child || !start) {
    return;
  }

  if (end_child != start) {
    walk_down_entry(start, end_child->config->parent);
  }

  if (end_child && end_child->config->entry_fn) {
    end_child->config->entry_fn(end_child);
  }
}

/** \brief Walks down the initial states and sets them active. Returns the reached leaf. */
static State *walk_down_init(State *start) {
  for (; start->config->initial != NULL; start = start->config->initial) {
    start->_active = start->config->initial;
  }
  start->_active = start->config->initial;
  return start;
}

/** \brief Finds the current active leaf in a branch. */
static State *find_leaf(State const *start) {
  State const *leaf;
  for (leaf = start; leaf->_active != NULL; leaf = leaf->_active) {
  }
  return (State *)leaf;
}

/** \brief Finds the root of the statechart. */
static State *find_root(State const *start) {
  State const *root = NULL;
  for (root = start; root->config->parent != NULL; root = root->config->parent) {
  }
  return (State *)root;
}

/** \brief Finds a valid (matching or automatic) transition in the active branch. */
static Transition const *find_transition(State const *const root, EventType event) {
  Transition const *transitions = root->config->transitions;

  for (State const *s = root->_active; s != NULL; s = s->config->parent) {
    if (!s->config->transitions) {
      continue;
    }
    transitions = s->config->transitions;
    for (Transition const *t = transitions; t->type != SC_TTYPE_TABLE_END; ++t) {
      if (t->event == event || t->event == SC_NO_EVENT) {
        if (transitions != root->config->transitions) {
          if (!t->guard_fn || (t->guard_fn && t->guard_fn(root))) {
            return t;
          }
        } else {
          // Search from current state to root. TODO: Only needed when table is not sourced by state
          for (State const *parent = root->_active; parent != NULL;
               parent = parent->config->parent) {
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

/** \brief Runs a state. Returns the requested target state, NULL otherwise.
 * Returning the running state itself or the current leaf means "no change". */
static State *run_state(State const *const root, State *s, EventType e) {
  if (s->config->run_fn) {
    State *target_state = s->config->run_fn(s, e);
    if (target_state && target_state != s && target_state != root->_active) {
      return target_state;
    }
  }
  return NULL;
}

/** \brief See run_state. Does this for all states of the active branch, leaf first. */
static State *ancestors_run(State const *root, EventType event) {
  for (State *s = root->_active; s != NULL; s = s->config->parent) {
    State *requested_state = run_state(root, s, event);
    if (requested_state) {
      return requested_state;
    }
  }
  return NULL;
}

/** \brief Last active direct child of parent. The root stores the active leaf instead. */
static State *active_child(State const *parent) {
  if (parent->config->parent != NULL) {
    return parent->_active;
  }
  State *s = parent->_active;
  while (s != NULL && s->config->parent != parent) {
    s = s->config->parent;
  }
  return s;
}

/** \brief Depending on target state type, return the real target state. */
static State *get_target_state_from_type(State *to) {
  State *const parent = to->config->parent;
  State *fallback = NULL;
  State *target_state = NULL;
  switch (to->config->type) {
  case SC_TYPE_NORMAL:
  case SC_TYPE_CHOICE:
    target_state = to;
    break;
  case SC_TYPE_HISTORY:
  case SC_TYPE_HISTORY_DEEP:
    fallback = to->config->initial ? to->config->initial : parent->config->initial;
    if (to->config->type == SC_TYPE_HISTORY) {
      target_state = active_child(parent);
    } else {
      target_state = find_leaf(parent);
      if (target_state == parent) {
        target_state = NULL;
      }
    }
    if (!target_state) {
      target_state = fallback;
    }
    break;
  case SC_TYPE_ROOT:
    target_state = NULL;
    break;
  }
  return target_state;
}

/** \brief Executes a single transition from the active leaf. */
static void execute_transition(State *root, State const *from, State *to, TransitionType type,
                               transition_fn action) {
  // Handle StateType
  State *target_state = get_target_state_from_type(to);

  // Internal transition (local, same -> same): no exit/entry, configuration unchanged.
  if (type == SC_TTYPE_LOCAL && from == target_state) {
    if (action) {
      action(root);
    }
    return;
  }

  // Find common ancestor of source and target
  State *ca = fca(from, target_state);

  State *from_leaf = root->_active;

  // Set target branch active states to reach target
  walk_up_set_active_state(target_state, ca);

  // Local transitions do not exit the containing state
  if (type == SC_TTYPE_LOCAL) {
    ca = ca->_active;
  }

  // Exit all states on the active branch until ancestor
  walk_up_exit(from_leaf, ca);

  // Transition
  if (action) {
    action(root);
  }

  // Enter target branch incl. target (target not re-entered if it is the local container)
  if (ca != target_state) {
    walk_down_entry(ca->_active, target_state);
  }

  // The target might not be initialized yet
  State *leaf = walk_down_init(target_state);

  // Enter initial states down to the leaf
  walk_down_entry(target_state->_active, leaf);
  root->_active = leaf;
}

/* -------- Public -------- */

State const *sc_init(State *root) {
  root->_active = walk_down_init(root);
  walk_down_entry(root, root->_active);
  return root->_active;
}

void sc_map_stateconfig_to_states(size_t num_states, State states[num_states],
                                  StateConfig const statecfgs[num_states]) {
  for (size_t i = 0; i < num_states; ++i) {
    states[i].config = &statecfgs[i];
  }
}

void sc_reset_state(State *state) { state->_active = NULL; }

State const *sc_get_root(State const *s) { return find_root(s); }

State const *sc_run(State *root, EventType event) {
  Transition const *t = find_transition(root, event);
  State *run_target = t ? NULL : ancestors_run(root, event);

  while (t || run_target) {
    if (t) {
      execute_transition(root, t->from, t->to, t->type, t->transition_fn);
    } else {
      execute_transition(root, root->_active, run_target, SC_TTYPE_EXTERNAL, NULL);
    }

    // Check automatic transitions of the new active branch
    t = find_transition(root, SC_NO_EVENT);

    // Run all run functions of the active branch, continue if a change is requested
    run_target = t ? NULL : ancestors_run(root, event);
  }

  return root->_active;
}
