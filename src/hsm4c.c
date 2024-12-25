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
static hsm4c_state_t *fca(hsm4c_state_t const *const left, hsm4c_state_t const *const right) {
  for (hsm4c_state_t const *_left = left; _left->config->parent != NULL;
       _left = _left->config->parent) {
    for (hsm4c_state_t const *_right = right; _right->config->parent != NULL;
         _right = _right->config->parent) {
      if (_left->config->parent == _right->config->parent) {
        return _left->config->parent;
      }
    }
  }
  return NULL;
}

/** \brief Walk up a branch and call exit_fn(). end_ancestor MUST be a valid ancestor or NULL. */
static void walk_up_exit(hsm4c_state_t const *const root, hsm4c_state_t const *start,
                         hsm4c_state_t const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    if (start->config->exit_fn) {
      start->config->exit_fn(start);
    }
  }
}

/** \brief Walk up a branch and set active state to match the branch. end_ancestor MUST be a valid
 * ancestor or NULL. */
static void walk_up_set_active_state(hsm4c_state_t *start, hsm4c_state_t const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    start->config->parent->_active = start;
  }
}

/** \brief Walk down a branch and call entry_fn(). */
static void walk_down_entry(hsm4c_state_t const *const start,
                            hsm4c_state_t const *const end_child) {
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

/** \brief Walk down a branch and set initial state to active if present. */
static hsm4c_state_t *walk_down_init(hsm4c_state_t *start) {
  for (; start->config->initial != NULL; start = start->config->initial) {
    start->_active = start->config->initial;
  }
  start->_active = start->config->initial;
  return start;
}

/** \brief Finds current active leaf in a branch. */
static hsm4c_state_t *find_leaf(hsm4c_state_t const *start) {
  hsm4c_state_t const *leaf;
  for (leaf = start; leaf->_active != NULL; leaf = leaf->_active) {
  }
  return (hsm4c_state_t *)leaf;
}

/** \brief Finds root of statechart */
static hsm4c_state_t *find_root(hsm4c_state_t const *start) {
  hsm4c_state_t const *root = NULL;
  for (root = start; root->config->parent != NULL; root = root->config->parent) {
  }
  return (hsm4c_state_t *)root;
}

/** \brief Finds valid (matching or automatic) transition in active branch. */
static hsm4c_transition_t const *find_transition(hsm4c_state_t const *const root,
                                                 hsm4c_event_t event) {
  hsm4c_transition_t const *transitions = root->config->transitions;

  for (hsm4c_state_t const *s = root->_active; s != NULL; s = s->config->parent) {
    if (!s->config->transitions) {
      continue;
    }
    transitions = s->config->transitions;
    for (hsm4c_transition_t const *t = transitions; t->type != HSM4C_TTYPE_TABLE_END; ++t) {
      if (t->event == event || t->event == HSM4C_NO_EVENT) {
        if (transitions != root->config->transitions) {
          if (!t->guard_fn || t->guard_fn(root)) {
            return t;
          }
        } else {
          // Search from current state to root. TODO: Only needed when table is not sourced by state
          for (hsm4c_state_t const *parent = root->_active; parent != NULL;
               parent = parent->config->parent) {
            if (t->from == parent) {
              if (!t->guard_fn || t->guard_fn(root)) {
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
static hsm4c_state_t *run_state(hsm4c_state_t const *const root, hsm4c_state_t *s,
                                hsm4c_event_t e) {
  if (s->config->run_fn) {
    hsm4c_state_t *target_state = s->config->run_fn(s, e);
    if (target_state && target_state != root->_active) {
      return target_state;
    }
  }
  return NULL;
}

/** \brief See run_state. Do this for all states in a branch.  */
static hsm4c_state_t *ancestors_run(hsm4c_state_t const *root, hsm4c_event_t event) {
  for (hsm4c_state_t *s = root->_active; s != NULL; s = s->config->parent) {
    hsm4c_state_t *requested_state = run_state(root, s, event);
    if (requested_state) {
      return requested_state;
    }
  }
  return NULL;
}

/** \brief Depending on active state type, return target state. */
static hsm4c_state_t *get_target_state_from_type(hsm4c_transition_t const *const t) {
  hsm4c_state_t *target_state = NULL;
  switch (t->to->config->type) {
  case HSM4C_TYPE_NORMAL:
  case HSM4C_TYPE_CHOICE:
    target_state = t->to;
    break;
  case HSM4C_TYPE_HISTORY:
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
  case HSM4C_TYPE_HISTORY_DEEP:
    target_state = find_leaf(t->to->config->parent);
    if (target_state == t->to->config->parent) {
      target_state = t->to->config->initial;
    }
    break;
  case HSM4C_TYPE_ROOT:
    target_state = NULL;
    break;
  }
  return target_state;
}

/* -------- Public -------- */

hsm4c_state_t const *hsm4c_init(hsm4c_state_t *root) {
  root->_active = walk_down_init(root);
  walk_down_entry(root, root->_active);
  return root->_active;
}

void hsm4c_assign_stateconfigs_to_states(size_t num_states, hsm4c_state_t *states,
                                         hsm4c_state_config_t const *statecfgs) {
  for (size_t i = 0; i < num_states; ++i) {
    states[i].config = &statecfgs[i];
  }
}

void hsm4c_reset_state(hsm4c_state_t *state) { state->_active = NULL; }

hsm4c_state_t const *hsm4c_get_root(hsm4c_state_t const *s) { return find_root(s); }

hsm4c_state_t const *hsm4c_run(hsm4c_state_t *root, hsm4c_event_t event) {
  hsm4c_transition_t const *t = find_transition(root, event);

  if (!t) {
    hsm4c_state_t *requested_state = ancestors_run(root, event);
    if (requested_state) {
      t = &(hsm4c_transition_t const){.from = root->_active, .to = requested_state};
    }
  }

  while (t) {
    // Handle hsm4c_StateType
    hsm4c_state_t *target_state = get_target_state_from_type(t);

    // Find common ancestor of active leaf and target
    hsm4c_state_t *ca = fca(t->from, target_state);

    hsm4c_state_t *from_leaf = NULL;

    from_leaf = root->_active;

    // Set target branch active states to reach target
    walk_up_set_active_state(target_state, ca);

    // Exit all states on the active branch until ancestor
    if (t->type == HSM4C_TTYPE_LOCAL) {
      ca = ca->_active;
    }
    walk_up_exit(root, from_leaf, ca);

    // hsm4c_Transition
    if (t->transition_fn)
      t->transition_fn(root);

    // Entry target branch incl. target
    walk_down_entry(ca->_active, target_state);

    // We might have not initialized this state yet
    from_leaf = walk_down_init(target_state);

    // Walk down until leaf
    walk_down_entry(target_state->_active, from_leaf);
    root->_active = from_leaf ? from_leaf : target_state;
    t = NULL;

    // Check transitions of current state with no event
    t = find_transition(root, HSM4C_NO_EVENT);

    // Run all "run" functions including parents, continue change if requested
    if (!t) {
      hsm4c_state_t *requested_state = ancestors_run(root, event);
      if (requested_state) {
        t = &(hsm4c_transition_t const){.from = root->_active, .to = requested_state};
      }
    }
  }

  return root->_active;
}
