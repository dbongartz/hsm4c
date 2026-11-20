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

/** \brief Finds the first enabled transition with source s and the given event in a table. */
static Transition const *match_table(Transition const *table, State const *s,
                                     State const *root, EventType event) {
  if (!table) {
    return NULL;
  }
  for (Transition const *t = table; t->type != SC_TTYPE_TABLE_END; ++t) {
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
static Transition const *find_transition(State const *const root, EventType event) {
  for (State const *s = root->_active; s != NULL; s = s->config->parent) {
    Transition const *t = match_table(s->config->transitions, s, root, event);
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

/** \brief Returns true if s is a proper descendant of ancestor. */
static bool is_descendant(State const *s, State const *ancestor) {
  for (s = s->config->parent; s != NULL; s = s->config->parent) {
    if (s == ancestor) {
      return true;
    }
  }
  return false;
}

/** \brief Executes a single transition from the active leaf. */
static void execute_transition(State *root, State *from, State *to, TransitionType type,
                               transition_fn action) {
  // Handle StateType
  State *target_state = to ? get_target_state_from_type(to) : NULL;

  // Targetless or local "Same -> Same": internal transition. No exit/entry, nothing changes.
  if (!target_state || (type == SC_TTYPE_LOCAL && from == target_state)) {
    if (action) {
      action(root);
    }
    return;
  }

  // Transition domain: A local transition into a descendant does not exit its source.
  // Otherwise (also local with any other target) the common ancestor of source and target.
  State *domain = (type == SC_TTYPE_LOCAL && is_descendant(target_state, from))
                      ? from
                      : fca(from, target_state);

  State *from_leaf = root->_active;

  // Set target branch active states to reach target
  walk_up_set_active_state(target_state, domain);

  // Exit all states on the active branch below the domain
  walk_up_exit(from_leaf, domain);

  // Transition
  if (action) {
    action(root);
  }

  // Enter target branch incl. target
  walk_down_entry(domain->_active, target_state);

  // The target might not be initialized yet
  State *leaf = walk_down_init(target_state);

  // Enter initial states down to the leaf
  walk_down_entry(target_state->_active, leaf);
  root->_active = leaf;
}

/** \brief Executes a transition table row. */
static void take_transition(State *root, Transition const *t) {
  execute_transition(root, t->from, t->to, t->type, t->transition_fn);
}

/**
 * \brief Takes automatic transitions until none is enabled.
 *
 * With run_fns, run functions are called whenever no automatic transition is enabled and can
 * request further transitions.
 */
static void settle(State *root, EventType event, bool run_fns) {
  for (;;) {
    Transition const *t = find_transition(root, SC_NO_EVENT);
    if (t) {
      take_transition(root, t);
      continue;
    }
    if (!run_fns) {
      return;
    }
    State *run_target = ancestors_run(root, event);
    if (!run_target) {
      return;
    }
    execute_transition(root, root->_active, run_target, SC_TTYPE_EXTERNAL, NULL);
  }
}

/* -------- Public -------- */

State const *sc_init(State *root) {
  root->_active = walk_down_init(root);
  walk_down_entry(root, root->_active);
  settle(root, SC_NO_EVENT, false);
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
  // Event transitions first, automatic transitions afterwards (SCXML macrostep)
  Transition const *t = event != SC_NO_EVENT ? find_transition(root, event) : NULL;
  if (t) {
    take_transition(root, t);
  }

  settle(root, event, true);

  return root->_active;
}
