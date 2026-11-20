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
                               hsm4c_transition_type_t type, hsm4c_action_fn_t action,
                               hsm4c_event_t const *event) {
  hsm4c_state_id_t const target = to != HSM4C_NO_STATE ? resolve_target(sm, to) : HSM4C_NO_STATE;

  // Targetless or local "Same -> Same": internal transition. No exit/entry, nothing changes.
  if (target == HSM4C_NO_STATE || (type == HSM4C_TRANSITION_LOCAL && from == target)) {
    if (action) {
      action(sm, event);
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
    action(sm, event);
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
static hsm4c_transition_t const *find_transition(hsm4c_t const *sm, hsm4c_event_id_t id,
                                                 hsm4c_event_t const *event,
                                                 hsm4c_state_id_t *source) {
  hsm4c_state_id_t s = sm->leaf;
  for (;;) {
    hsm4c_transition_t const *t = cfg(sm, s)->transitions;
    for (; t && t->type != HSM4C_TRANSITION_TABLE_END; ++t) {
      if (t->event == id && (!t->guard_fn || t->guard_fn(sm, event))) {
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

/**
 * \brief Calls the run functions of the active branch, leaf first.
 *
 * \return The first requested state change, HSM4C_NO_STATE if none.
 */
static hsm4c_state_id_t run_states(hsm4c_t *sm, hsm4c_event_t const *event) {
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
 * \brief Executes a transition, counted against HSM4C_MAX_TRANSITIONS.
 *
 * \return HSM4C_ERR_LIMIT without executing it if the limit is reached.
 */
static hsm4c_result_t take(hsm4c_t *sm, hsm4c_state_id_t from, hsm4c_state_id_t to,
                           hsm4c_transition_type_t type, hsm4c_action_fn_t action,
                           hsm4c_event_t const *event, unsigned *taken) {
  // Stop endless chains of transitions, e.g. a cycle of always enabled automatic transitions
  if (*taken >= HSM4C_MAX_TRANSITIONS) {
    return HSM4C_ERR_LIMIT;
  }
  ++*taken;
  execute_transition(sm, from, to, type, action, event);
  return HSM4C_OK;
}

/** \brief Takes the transition for event, if one is enabled. */
static hsm4c_result_t take_event(hsm4c_t *sm, hsm4c_event_t const *event, unsigned *taken) {
  hsm4c_state_id_t source = HSM4C_ROOT;
  hsm4c_transition_t const *const t = find_transition(sm, event->id, event, &source);
  if (!t) {
    return HSM4C_OK;
  }
  return take(sm, source, t->to, t->type, t->action_fn, event, taken);
}

/**
 * \brief Takes automatic transitions and internal events until none is left.
 *
 * Order per step (SCXML): an enabled automatic transition, else the oldest internal event.
 * With run_fns, run functions are called when neither is left and can request further
 * transitions.
 *
 * \param event   Last processed event, passed to the callbacks. Updated by internal events.
 * \param taken   Transitions taken in this call, counted against HSM4C_MAX_TRANSITIONS.
 */
static hsm4c_result_t settle(hsm4c_t *sm, hsm4c_event_t *event, bool run_fns, unsigned *taken) {
  for (;;) {
    hsm4c_state_id_t source = HSM4C_ROOT;
    hsm4c_transition_t const *const t = find_transition(sm, HSM4C_NO_EVENT, event, &source);
    if (t) {
      hsm4c_result_t const r = take(sm, source, t->to, t->type, t->action_fn, event, taken);
      if (r != HSM4C_OK) {
        return r;
      }
      continue;
    }

    if (sm->queue_len > 0) {
      hsm4c_event_t const next = sm->queue[sm->queue_head];
      hsm4c_result_t const r = take_event(sm, &next, taken);
      if (r != HSM4C_OK) {
        return r; // The event stays queued
      }
      sm->queue_head = (uint8_t)((sm->queue_head + 1U) % HSM4C_EVENT_QUEUE_SIZE);
      --sm->queue_len;
      *event = next;
      continue;
    }

    if (!run_fns) {
      return HSM4C_OK;
    }
    hsm4c_state_id_t const run_target = run_states(sm, event);
    if (run_target == HSM4C_NO_STATE) {
      return HSM4C_OK;
    }
    hsm4c_result_t const r =
        take(sm, sm->leaf, run_target, HSM4C_TRANSITION_EXTERNAL, NULL, event, taken);
    if (r != HSM4C_OK) {
      return r;
    }
  }
}

/* -------- Public -------- */

hsm4c_result_t hsm4c_init(hsm4c_t *sm, hsm4c_chart_t const *chart, hsm4c_state_id_t *active,
                          void *ctx) {
  sm->chart = chart;
  sm->active = active;
  sm->ctx = ctx;
  sm->queue_head = 0;
  sm->queue_len = 0;
  sm->busy = true;
  for (hsm4c_state_id_t i = 0; i < chart->num_states; ++i) {
    active[i] = HSM4C_NO_STATE;
  }

  enter(sm, HSM4C_ROOT);
  sm->leaf = enter_initial(sm, HSM4C_ROOT);

  hsm4c_event_t current = {HSM4C_NO_EVENT, NULL};
  unsigned taken = 0;
  hsm4c_result_t const r = settle(sm, &current, false, &taken);
  sm->busy = false;
  return r;
}

hsm4c_result_t hsm4c_run(hsm4c_t *sm, hsm4c_event_id_t event, void *data) {
  if (sm->busy) {
    return HSM4C_ERR_BUSY;
  }
  sm->busy = true;

  hsm4c_event_t current = {HSM4C_NO_EVENT, NULL};
  unsigned taken = 0;
  hsm4c_result_t r = HSM4C_OK;

  // Internal events raised outside of a call come before the new event (SCXML)
  if (sm->queue_len > 0) {
    r = settle(sm, &current, false, &taken);
  }

  // The event's transition first, then automatic transitions and internal events
  if (r == HSM4C_OK) {
    current.id = event;
    current.data = data;
    if (event != HSM4C_NO_EVENT) {
      r = take_event(sm, &current, &taken);
    }
  }
  if (r == HSM4C_OK) {
    r = settle(sm, &current, true, &taken);
  }

  sm->busy = false;
  return r;
}

hsm4c_result_t hsm4c_raise(hsm4c_t *sm, hsm4c_event_id_t event, void *data) {
  if (sm->queue_len >= HSM4C_EVENT_QUEUE_SIZE) {
    return HSM4C_ERR_QUEUE_FULL;
  }
  unsigned const tail = (sm->queue_head + sm->queue_len) % HSM4C_EVENT_QUEUE_SIZE;
  sm->queue[tail].id = event;
  sm->queue[tail].data = data;
  ++sm->queue_len;
  return HSM4C_OK;
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
