/**
 * \brief Implementation of statechart driver
 * \file
 *
 * (C) 2023 David Bongartz
 * MIT License
 */

#include "hsm4c/hsm4c.h"

#include <stdbool.h>
#include <stddef.h>

/* -------- Private -------- */

/** \brief Finds the common ancestor of two states. Both must be in the same tree. */
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

/** \brief Walks up a branch and calls exit_fn(). end_ancestor MUST be a valid ancestor or NULL. */
static void walk_up_exit(hsm4c_state_t const *start, hsm4c_state_t const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    if (start->config->exit_fn) {
      start->config->exit_fn(start);
    }
  }
}

/** \brief Walks up a branch and sets the active states to match the branch. end_ancestor MUST be
 * a valid ancestor or NULL. */
static void walk_up_set_active_state(hsm4c_state_t *start, hsm4c_state_t const *end_ancestor) {
  for (; start != end_ancestor; start = start->config->parent) {
    start->config->parent->_active = start;
  }
}

/** \brief Walks down a branch from start to end_child and calls entry_fn(). */
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

/** \brief Walks down the initial states and sets them active. Returns the reached leaf. */
static hsm4c_state_t *walk_down_init(hsm4c_state_t *start) {
  for (; start->config->initial != NULL; start = start->config->initial) {
    start->_active = start->config->initial;
  }
  start->_active = start->config->initial;
  return start;
}

/** \brief Finds the current active leaf in a branch. */
static hsm4c_state_t *find_leaf(hsm4c_state_t const *start) {
  hsm4c_state_t const *leaf;
  for (leaf = start; leaf->_active != NULL; leaf = leaf->_active) {
  }
  return (hsm4c_state_t *)leaf;
}

/** \brief Finds the root of the statechart. */
static hsm4c_state_t *find_root(hsm4c_state_t const *start) {
  hsm4c_state_t const *root = NULL;
  for (root = start; root->config->parent != NULL; root = root->config->parent) {
  }
  return (hsm4c_state_t *)root;
}

/** \brief Finds the first enabled transition with source s and the given event in a table. */
static hsm4c_transition_t const *match_table(hsm4c_transition_t const *table,
                                             hsm4c_state_t const *s, hsm4c_state_t const *root,
                                             hsm4c_event_id_t event) {
  if (!table) {
    return NULL;
  }
  for (hsm4c_transition_t const *t = table; t->type != HSM4C_TRANSITION_TABLE_END; ++t) {
    if (t->from == s && t->event == event && (!t->guard_fn || t->guard_fn(root))) {
      return t;
    }
  }
  return NULL;
}

/**
 * \brief Finds an enabled transition for event in the active branch.
 *
 * States are searched from the active leaf up to the root, so deeper states take priority.
 * For each state its own table is searched first, then the root's table.
 */
static hsm4c_transition_t const *find_transition(hsm4c_state_t const *const root,
                                                 hsm4c_event_id_t event) {
  for (hsm4c_state_t const *s = root->_active; s != NULL; s = s->config->parent) {
    hsm4c_transition_t const *t = match_table(s->config->transitions, s, root, event);
    if (!t && s != root) {
      t = match_table(root->config->transitions, s, root, event);
    }
    if (t) {
      return t;
    }
  }
  return NULL;
}

/** \brief Runs a state. Returns the requested target state, NULL otherwise.
 * Returning the running state itself or the current leaf means "no change". */
static hsm4c_state_t *run_state(hsm4c_state_t const *const root, hsm4c_state_t *s,
                                hsm4c_event_id_t e) {
  if (s->config->run_fn) {
    hsm4c_state_t *target_state = s->config->run_fn(s, e);
    if (target_state && target_state != s && target_state != root->_active) {
      return target_state;
    }
  }
  return NULL;
}

/** \brief See run_state. Does this for all states of the active branch, leaf first. */
static hsm4c_state_t *ancestors_run(hsm4c_state_t const *root, hsm4c_event_id_t event) {
  for (hsm4c_state_t *s = root->_active; s != NULL; s = s->config->parent) {
    hsm4c_state_t *requested_state = run_state(root, s, event);
    if (requested_state) {
      return requested_state;
    }
  }
  return NULL;
}

/** \brief Last active direct child of parent. The root stores the active leaf instead. */
static hsm4c_state_t *active_child(hsm4c_state_t const *parent) {
  if (parent->config->parent != NULL) {
    return parent->_active;
  }
  hsm4c_state_t *s = parent->_active;
  while (s != NULL && s->config->parent != parent) {
    s = s->config->parent;
  }
  return s;
}

/** \brief Depending on target state type, return the real target state. */
static hsm4c_state_t *get_target_state_from_type(hsm4c_state_t *to) {
  hsm4c_state_t *const parent = to->config->parent;
  hsm4c_state_t *fallback = NULL;
  hsm4c_state_t *target_state = NULL;
  switch (to->config->type) {
  case HSM4C_STATE_NORMAL:
  case HSM4C_STATE_CHOICE:
    target_state = to;
    break;
  case HSM4C_STATE_HISTORY:
  case HSM4C_STATE_HISTORY_DEEP:
    fallback = to->config->initial ? to->config->initial : parent->config->initial;
    if (to->config->type == HSM4C_STATE_HISTORY) {
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
  case HSM4C_STATE_ROOT:
    target_state = NULL;
    break;
  }
  return target_state;
}

/** \brief Returns true if s is a proper descendant of ancestor. */
static bool is_descendant(hsm4c_state_t const *s, hsm4c_state_t const *ancestor) {
  for (s = s->config->parent; s != NULL; s = s->config->parent) {
    if (s == ancestor) {
      return true;
    }
  }
  return false;
}

/** \brief Executes a single transition from the active leaf. */
static void execute_transition(hsm4c_state_t *root, hsm4c_state_t *from, hsm4c_state_t *to,
                               hsm4c_transition_type_t type, hsm4c_transition_fn_t action) {
  // Handle hsm4c_state_type_t
  hsm4c_state_t *target_state = to ? get_target_state_from_type(to) : NULL;

  // Targetless or local "Same -> Same": internal transition. No exit/entry, nothing changes.
  if (!target_state || (type == HSM4C_TRANSITION_LOCAL && from == target_state)) {
    if (action) {
      action(root);
    }
    return;
  }

  // hsm4c_transition_t domain: A local transition into a descendant does not exit its source.
  // Otherwise (also local with any other target) the common ancestor of source and target.
  hsm4c_state_t *domain = (type == HSM4C_TRANSITION_LOCAL && is_descendant(target_state, from))
                              ? from
                              : fca(from, target_state);

  // The root is never exited, e.g. for transitions with the root as source
  if (!domain) {
    domain = root;
  }

  hsm4c_state_t *from_leaf = root->_active;

  // Set target branch active states to reach target
  walk_up_set_active_state(target_state, domain);

  // Exit all states on the active branch below the domain
  walk_up_exit(from_leaf, domain);

  // hsm4c_transition_t
  if (action) {
    action(root);
  }

  // Enter target branch incl. target
  walk_down_entry(domain->_active, target_state);

  // The target might not be initialized yet
  hsm4c_state_t *leaf = walk_down_init(target_state);

  // Enter initial states down to the leaf
  walk_down_entry(target_state->_active, leaf);
  root->_active = leaf;
}

/** \brief Executes a transition table row. */
static void take_transition(hsm4c_state_t *root, hsm4c_transition_t const *t) {
  execute_transition(root, t->from, t->to, t->type, t->transition_fn);
}

/**
 * \brief Takes automatic transitions until none is enabled.
 *
 * With run_fns, run functions are called whenever no automatic transition is enabled and can
 * request further transitions.
 */
static void settle(hsm4c_state_t *root, hsm4c_event_id_t event, bool run_fns) {
  for (;;) {
    hsm4c_transition_t const *t = find_transition(root, HSM4C_NO_EVENT);
    if (t) {
      take_transition(root, t);
      continue;
    }
    if (!run_fns) {
      return;
    }
    hsm4c_state_t *run_target = ancestors_run(root, event);
    if (!run_target) {
      return;
    }
    execute_transition(root, root->_active, run_target, HSM4C_TRANSITION_EXTERNAL, NULL);
  }
}

/* -------- Public -------- */

hsm4c_state_t const *hsm4c_init(hsm4c_state_t *root) {
  root->_active = walk_down_init(root);
  walk_down_entry(root, root->_active);
  settle(root, HSM4C_NO_EVENT, false);
  return root->_active;
}

void hsm4c_map_stateconfig_to_states(size_t num_states, hsm4c_state_t states[num_states],
                                     hsm4c_state_config_t const statecfgs[num_states]) {
  for (size_t i = 0; i < num_states; ++i) {
    states[i].config = &statecfgs[i];
  }
}

void hsm4c_reset_state(hsm4c_state_t *state) { state->_active = NULL; }

hsm4c_state_t const *hsm4c_get_root(hsm4c_state_t const *s) { return find_root(s); }

hsm4c_state_t const *hsm4c_run(hsm4c_state_t *root, hsm4c_event_id_t event) {
  // Event transitions first, automatic transitions afterwards (SCXML macrostep)
  hsm4c_transition_t const *t = event != HSM4C_NO_EVENT ? find_transition(root, event) : NULL;
  if (t) {
    take_transition(root, t);
  }

  settle(root, event, true);

  return root->_active;
}
