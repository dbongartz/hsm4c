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

/** \brief Configuration of a state. */
static hsm4c_state_t const *cfg(hsm4c_t const *sm, hsm4c_state_id_t id) {
  return &sm->chart->states[id];
}

/** \brief Parent of a state. Must not be called for the root. */
static hsm4c_state_id_t parent_of(hsm4c_t const *sm, hsm4c_state_id_t id) {
  return cfg(sm, id)->parent;
}

/** \brief Returns true if s is a proper descendant of ancestor. */
static bool is_descendant(hsm4c_t const *sm, hsm4c_state_id_t s, hsm4c_state_id_t ancestor) {
  while (s != HSM4C_ROOT) {
    s = parent_of(sm, s);
    if (s == ancestor) {
      return true;
    }
  }
  return false;
}

/**
 * \brief Least common compound ancestor: the nearest proper ancestor of source that is also a
 * proper ancestor of target. The root if source is the root.
 */
static hsm4c_state_id_t lcca(hsm4c_t const *sm, hsm4c_state_id_t source, hsm4c_state_id_t target) {
  hsm4c_state_id_t s = source;
  while (s != HSM4C_ROOT) {
    s = parent_of(sm, s);
    if (is_descendant(sm, target, s)) {
      return s;
    }
  }
  return HSM4C_ROOT;
}

/** \brief Finds the active (or last active) leaf below a state. */
static hsm4c_state_id_t find_leaf(hsm4c_t const *sm, hsm4c_state_id_t s) {
  while (sm->active[s] != HSM4C_NO_STATE) {
    s = sm->active[s];
  }
  return s;
}

/** \brief Calls the entry function of a state, if any. */
static void enter(hsm4c_t *sm, hsm4c_state_id_t s) {
  if (cfg(sm, s)->entry_fn) {
    cfg(sm, s)->entry_fn(sm, s);
  }
}

/** \brief Calls the exit function of a state, if any. */
static void leave(hsm4c_t *sm, hsm4c_state_id_t s) {
  if (cfg(sm, s)->exit_fn) {
    cfg(sm, s)->exit_fn(sm, s);
  }
}

/**
 * \brief Enters the initial states below s down to a leaf and sets them active.
 *
 * \return The reached leaf.
 */
static hsm4c_state_id_t enter_initial(hsm4c_t *sm, hsm4c_state_id_t s) {
  while (cfg(sm, s)->initial != HSM4C_NO_STATE) {
    sm->active[s] = cfg(sm, s)->initial;
    s = cfg(sm, s)->initial;
    enter(sm, s);
  }
  sm->active[s] = HSM4C_NO_STATE;
  return s;
}

/** \brief Depending on the target's type, returns the state to actually enter. */
static hsm4c_state_id_t resolve_target(hsm4c_t const *sm, hsm4c_state_id_t to) {
  hsm4c_state_t const *const to_cfg = cfg(sm, to);
  hsm4c_state_id_t const parent = to_cfg->parent;
  hsm4c_state_id_t target = to;

  if (to_cfg->type == HSM4C_STATE_HISTORY) {
    target = sm->active[parent];
  } else if (to_cfg->type == HSM4C_STATE_HISTORY_DEEP) {
    target = find_leaf(sm, parent);
    if (target == parent) {
      target = HSM4C_NO_STATE;
    }
  }

  if (target == HSM4C_NO_STATE) {
    // History without history: own initial, then the parent's
    target = to_cfg->initial != HSM4C_NO_STATE ? to_cfg->initial : cfg(sm, parent)->initial;
  }
  return target;
}

/** \brief Executes a single transition from the active leaf. */
static void execute_transition(hsm4c_t *sm, hsm4c_state_id_t from, hsm4c_state_id_t to,
                               hsm4c_transition_type_t type, hsm4c_action_fn_t action) {
  hsm4c_state_id_t const target = to != HSM4C_NO_STATE ? resolve_target(sm, to) : HSM4C_NO_STATE;

  // Targetless or local "Same -> Same": internal transition. No exit/entry, nothing changes.
  if (target == HSM4C_NO_STATE || (type == HSM4C_TRANSITION_LOCAL && from == target)) {
    if (action) {
      action(sm);
    }
    return;
  }

  // Transition domain: a local transition into a descendant does not exit its source.
  // Otherwise (also local with any other target) the common ancestor of source and target.
  hsm4c_state_id_t const domain =
      (type == HSM4C_TRANSITION_LOCAL && is_descendant(sm, target, from)) ? from
                                                                          : lcca(sm, from, target);

  // Exit all states on the active branch below the domain, leaf first
  for (hsm4c_state_id_t s = sm->leaf; s != domain; s = parent_of(sm, s)) {
    leave(sm, s);
  }

  // Set the target branch active below the domain
  for (hsm4c_state_id_t s = target; s != domain; s = parent_of(sm, s)) {
    sm->active[parent_of(sm, s)] = s;
  }

  // Transition
  if (action) {
    action(sm);
  }

  // Enter the target branch down to the target
  hsm4c_state_id_t s = domain;
  do {
    s = sm->active[s];
    enter(sm, s);
  } while (s != target);

  // Enter initial states down to a leaf
  sm->leaf = enter_initial(sm, target);
}

/** \brief Finds an enabled transition for event in the active branch, deepest state first. */
static hsm4c_transition_t const *find_transition(hsm4c_t const *sm, hsm4c_event_id_t event,
                                                 hsm4c_state_id_t *source) {
  hsm4c_state_id_t s = sm->leaf;
  for (;;) {
    hsm4c_transition_t const *t = cfg(sm, s)->transitions;
    for (; t && t->type != HSM4C_TRANSITION_TABLE_END; ++t) {
      if (t->event == event && (!t->guard_fn || t->guard_fn(sm))) {
        *source = s;
        return t;
      }
    }
    if (s == HSM4C_ROOT) {
      return NULL;
    }
    s = parent_of(sm, s);
  }
}

/** \brief Takes the enabled transition for event, if any. Returns true if one was taken. */
static bool take_transition(hsm4c_t *sm, hsm4c_event_id_t event) {
  hsm4c_state_id_t source = HSM4C_ROOT;
  hsm4c_transition_t const *const t = find_transition(sm, event, &source);
  if (!t) {
    return false;
  }
  execute_transition(sm, source, t->to, t->type, t->action_fn);
  return true;
}

/**
 * \brief Calls the run functions of the active branch, leaf first.
 *
 * \return The first requested state change, HSM4C_NO_STATE if none.
 */
static hsm4c_state_id_t run_states(hsm4c_t *sm, hsm4c_event_id_t event) {
  hsm4c_state_id_t s = sm->leaf;
  for (;;) {
    if (cfg(sm, s)->run_fn) {
      hsm4c_state_id_t const target = cfg(sm, s)->run_fn(sm, s, event);
      if (target != HSM4C_NO_STATE && target != s && target != sm->leaf) {
        return target;
      }
    }
    if (s == HSM4C_ROOT) {
      return HSM4C_NO_STATE;
    }
    s = parent_of(sm, s);
  }
}

/**
 * \brief Takes automatic transitions until none is enabled.
 *
 * With run_fns, run functions are called whenever no automatic transition is enabled and can
 * request further transitions.
 */
static void settle(hsm4c_t *sm, hsm4c_event_id_t event, bool run_fns) {
  for (;;) {
    if (take_transition(sm, HSM4C_NO_EVENT)) {
      continue;
    }
    if (!run_fns) {
      return;
    }
    hsm4c_state_id_t const run_target = run_states(sm, event);
    if (run_target == HSM4C_NO_STATE) {
      return;
    }
    execute_transition(sm, sm->leaf, run_target, HSM4C_TRANSITION_EXTERNAL, NULL);
  }
}

/* -------- Public -------- */

hsm4c_state_id_t hsm4c_init(hsm4c_t *sm, hsm4c_chart_t const *chart, hsm4c_state_id_t *active,
                            void *ctx) {
  sm->chart = chart;
  sm->active = active;
  sm->ctx = ctx;
  for (hsm4c_state_id_t i = 0; i < chart->num_states; ++i) {
    active[i] = HSM4C_NO_STATE;
  }

  enter(sm, HSM4C_ROOT);
  sm->leaf = enter_initial(sm, HSM4C_ROOT);
  settle(sm, HSM4C_NO_EVENT, false);
  return sm->leaf;
}

hsm4c_state_id_t hsm4c_run(hsm4c_t *sm, hsm4c_event_id_t event) {
  // Event transitions first, automatic transitions afterwards (SCXML macrostep)
  if (event != HSM4C_NO_EVENT) {
    (void)take_transition(sm, event);
  }

  settle(sm, event, true);

  return sm->leaf;
}

void *hsm4c_ctx(hsm4c_t const *sm) { return sm->ctx; }

hsm4c_state_id_t hsm4c_current(hsm4c_t const *sm) { return sm->leaf; }

bool hsm4c_is_active(hsm4c_t const *sm, hsm4c_state_id_t state) {
  hsm4c_state_id_t s = sm->leaf;
  for (;;) {
    if (s == state) {
      return true;
    }
    if (s == HSM4C_ROOT) {
      return false;
    }
    s = parent_of(sm, s);
  }
}
