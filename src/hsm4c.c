#include "hsm4c.h"

#if HSM4C_CONFIG_LOG
  #include <stdio.h>
  #ifndef HSM4C_LOG
    #define HSM4C_LOG(...)                                    \
      do {                                                    \
        (void)fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
        (void)fprintf(stderr, __VA_ARGS__);                   \
        (void)fprintf(stderr, "\n");                          \
      } while (0)
  #endif
  #ifndef HSM4C_PRINT
    #define HSM4C_PRINT(...)                \
      do {                                  \
        (void)fprintf(stderr, __VA_ARGS__); \
      } while (0)
  #endif
#else
  #define HSM4C_LOG(...) \
    do {                 \
    } while (false)
#endif

static void hsm4c_cursor_down(hsm4c_t const *self, hsm4c_state_id_t *cursor) {
  HSM4C_ASSERT(self->cfg->states[*cursor].variant == HSM4C_STATE_COMPOUND);
  *cursor = self->cfg->states_rt[*cursor].compound.active_substate;
}

static void hsm4c_cursor_up(hsm4c_t const *self, hsm4c_state_id_t *cursor) {
  *cursor = self->cfg->states[*cursor].parent;
  *cursor = hsm4c_get_parent_state_id(self, *cursor);
}

static bool hsm4c_trigger_eq(hsm4c_trigger_t lhs, hsm4c_trigger_t rhs) {
  if (lhs.variant == HSM4C_TRIGGER_NORMAL) {
    return lhs.variant == rhs.variant && lhs.normal.id == rhs.normal.id;
  }

  if (lhs.variant == HSM4C_TRIGGER_COMPLETION_EVENT) {
    return lhs.variant == rhs.variant;
  }

  return false;
}

static inline bool hsm4c_has_guard(hsm4c_transition_t const *transition) {
#if HSM4C_CONFIG_TRANSITION_GUARDS
  return transition->guard_fn;
#else
  (void)transition;
  return false;
#endif
}

static inline bool hsm4c_check_guard(hsm4c_t const *self, hsm4c_transition_t const *transition,
                                     hsm4c_trigger_t trigger) {
#if HSM4C_CONFIG_TRANSITION_GUARDS
  if (hsm4c_has_guard(transition)) {
    return transition->guard_fn(self->cfg->ctx, trigger);
  }
  return true;

#else
  (void)self;
  (void)transition;
  (void)trigger;
  return true;
#endif
}

static inline bool hsm4c_transition_source_eq_state(hsm4c_transition_t const *trans,
                                                    hsm4c_state_id_t id) {
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  return trans->source == id;
#else
  (void)trans;
  (void)id;
  // For the non unified transition table, the source is the state itself
  return true;
#endif
}

static inline hsm4c_transition_table_t hsm4c_get_transition_table(hsm4c_t const *self,
                                                                  hsm4c_state_id_t state_id) {
  HSM4C_ASSERT(self->cfg->states[state_id].variant == HSM4C_STATE_COMPOUND);
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  return self->cfg->transitions;
#else
  return self->cfg->states[state_id].compound.transitions;
#endif
}

static hsm4c_transition_t const *hsm4c_find_first_matching_transition(hsm4c_t const *self,
                                                                      hsm4c_state_id_t state_id,
                                                                      hsm4c_trigger_t trigger) {
  static hsm4c_trigger_t const completion_trigger = {.variant = HSM4C_TRIGGER_COMPLETION_EVENT};
  hsm4c_transition_table_t const transitions = hsm4c_get_transition_table(self, state_id);

  struct match {
    bool found;
    size_t id;
  };

  struct match match_completion_only = {false, 0};
  struct match match_guard_only = {false, 0};
  struct match match_specific = {false, 0};

  for (size_t i = 0; i < transitions.num; ++i) {
    // Skip transitions that do not originate from the current state.
    if (!hsm4c_transition_source_eq_state(&transitions.entries[i], state_id)) {
      continue;
    }

    // Check for completion only events, the first one found is saved. We also abort the search here
    // as this is highest priority.
    if (!match_completion_only.found &&
        hsm4c_trigger_eq(transitions.entries[i].trigger, completion_trigger) &&
        !hsm4c_has_guard(&transitions.entries[i])) {
      match_completion_only.found = true;
      match_completion_only.id = i;
      break;
    }

    // Check for completion only events with a guard, the first one found is saved.
    if (!match_guard_only.found &&
        hsm4c_trigger_eq(transitions.entries[i].trigger, completion_trigger) &&
        hsm4c_has_guard(&transitions.entries[i]) &&
        hsm4c_check_guard(self, &transitions.entries[i], trigger)) {
      match_guard_only.found = true;
      match_guard_only.id = i;
    }

    // Check for specific trigger events, the first one found is saved.
    if (!match_specific.found && hsm4c_trigger_eq(transitions.entries[i].trigger, trigger) &&
        (!hsm4c_has_guard(&transitions.entries[i]) ||
         hsm4c_check_guard(self, &transitions.entries[i], trigger))) {
      match_specific.found = true;
      match_specific.id = i;
    }
  }

  // Prio 0: Completion only event without guard
  if (match_completion_only.found) {
    size_t tid = match_completion_only.id;
    HSM4C_LOG("%s: completion id: %zu", __func__, tid);
    return &transitions.entries[tid];
  }

  // Prio 1: Completion only event with guard
  if (match_guard_only.found) {
    size_t tid = match_guard_only.id;
    HSM4C_LOG("%s: completion guard id: %zu", __func__, tid);
    return &transitions.entries[tid];
  }

  // Prio 2: Specific trigger event
  if (match_specific.found) {
    size_t tid = match_specific.id;
    HSM4C_LOG("%s: specific id: %zu", __func__, tid);
    return &transitions.entries[tid];
  }

  HSM4C_LOG("%s: not found", __func__);
  return NULL;
}

#if HSM4C_CONFIG_STATE_NAME
static char const *hsm4c_get_name_or_addr(hsm4c_t const *self, hsm4c_state_id_t id) {
  return self->cfg->states[id].name;
}
#else
static char const *hsm4c_get_name_or_addr(hsm4c_t const *self, hsm4c_state_id_t id) {
  (void)self;
  #define MAX_PTR_STR_LEN (2 + (sizeof(void *) * 2))
  static char addr[MAX_PTR_STR_LEN];
  (void)snprintf(addr, MAX_PTR_STR_LEN, "%p", (void *)&self->cfg->states[id]);
  return addr;
}
#endif

static bool hsm4c_validate_cfg(hsm4c_cfg_t const *cfg) {
  if (cfg->states && cfg->states_rt && cfg->num_states >= 1) {
    return true;
  }

  return false;
}

static void hsm4c_call_entry(hsm4c_t const *self, hsm4c_state_id_t state_id) {
  void *const ctx = self->cfg->ctx;
  if (self->cfg->states[state_id].compound.entry_fn) {
    self->cfg->states[state_id].compound.entry_fn(ctx, &self->cfg->states[state_id]);
  }
}

static void hsm4c_call_exit(hsm4c_t const *self, hsm4c_state_id_t state_id) {
  void *const ctx = self->cfg->ctx;
  if (self->cfg->states[state_id].compound.exit_fn) {
    self->cfg->states[state_id].compound.exit_fn(ctx, &self->cfg->states[state_id]);
  }
}

static hsm4c_state_id_t hsm4c_find_common_ancestor(hsm4c_t const *self, hsm4c_state_id_t lhs,
                                                   hsm4c_state_id_t rhs) {
  hsm4c_state_id_t common_ancestor = HSM4C_STATE_ID_ZERO;

  hsm4c_state_id_t cursor_lhs = lhs;

  while (1) {
    hsm4c_cursor_up(self, &cursor_lhs);

    hsm4c_state_id_t cursor_rhs = rhs;

    while (1) {
      hsm4c_cursor_up(self, &cursor_rhs);
      if (cursor_rhs == HSM4C_STATE_ID_ZERO) {
        break;
      }

      if (cursor_lhs == cursor_rhs) {
        common_ancestor = cursor_lhs;
        break;
      }
    }

    if (cursor_lhs == HSM4C_STATE_ID_ZERO) {
      break;
    }
  }

  return common_ancestor;
}

static void hsm4c_walk_up_with_exit(hsm4c_t const *self, hsm4c_state_id_t from_incl,
                                    hsm4c_state_id_t to_excl) {
  hsm4c_state_id_t cursor = from_incl;

  while (1) {
    hsm4c_call_exit(self, cursor);
    hsm4c_cursor_up(self, &cursor);
    if (cursor == to_excl) {
      break;
    }
  }
}

static void hsm4c_walk_up_with_set_active_substate(hsm4c_t const *self, hsm4c_state_id_t from_incl,
                                                   hsm4c_state_id_t to_excl) {
  hsm4c_state_id_t cursor = from_incl;
  while (1) {
    hsm4c_state_id_t parent = hsm4c_get_parent_state_id(self, cursor);

    self->cfg->states_rt[parent].compound.active_substate = cursor;

    if (parent == to_excl) {
      break;
    }
    hsm4c_cursor_up(self, &cursor);
  }
}

static void hsm4c_walk_down_with_enter(hsm4c_t const *self, hsm4c_state_id_t from_excl,
                                       hsm4c_state_id_t to_incl) {
  hsm4c_state_id_t cursor = from_excl;

  while (1) {
    hsm4c_cursor_down(self, &cursor);
    hsm4c_call_entry(self, cursor);
    if (cursor == to_incl) {
      break;
    }
  }
}

static hsm4c_state_id_t hsm4c_walk_down_to_leaf_with_initial_and_enter(hsm4c_t const *self,
                                                                       hsm4c_state_id_t from_excl) {
  hsm4c_state_id_t active_state = from_excl;
  hsm4c_state_id_t cursor = from_excl;
  while (1) {
    active_state = cursor;
    self->cfg->states_rt[cursor].compound.active_substate =
        self->cfg->states[cursor].compound.initial;
    hsm4c_cursor_down(self, &cursor);
    if (cursor == HSM4C_STATE_ID_ZERO) {
      break;
    }
    hsm4c_call_entry(self, cursor);
  }

  return active_state;
}

/* -------- Public API -------- */

hsm4c_result_e hsm4c_start(hsm4c_t *self, hsm4c_cfg_t const *cfg) {
  HSM4C_ASSERT(self);
  HSM4C_ASSERT(cfg);

  if (!hsm4c_validate_cfg(cfg)) {
    return HSM4C_ERROR;
  }

  self->cfg = cfg;
  hsm4c_call_entry(self, HSM4C_STATE_ID_ZERO);
  self->active_state = hsm4c_walk_down_to_leaf_with_initial_and_enter(self, HSM4C_STATE_ID_ZERO);

  return HSM4C_OK;
}

hsm4c_state_t const *hsm4c_get_current_state(hsm4c_t const *self) {
  HSM4C_ASSERT(self);
  return &self->cfg->states[self->active_state];
}

hsm4c_result_e hsm4c_run2completion(hsm4c_t *self, hsm4c_trigger_t trigger) {
  HSM4C_ASSERT(self);

  hsm4c_transition_t const *transition = hsm4c_find_transition(self, trigger);

  if (!transition) {
    HSM4C_LOG("%s: no transition found\n", __func__);
    return HSM4C_OK;
  }

  while (1) {
    hsm4c_print_transition(self, transition);

    hsm4c_state_id_t common_ancestor =
        hsm4c_find_common_ancestor(self, self->active_state, transition->target);

    hsm4c_walk_up_with_exit(self, self->active_state, common_ancestor);

    hsm4c_walk_up_with_set_active_substate(self, transition->target, common_ancestor);

    hsm4c_walk_down_with_enter(self, common_ancestor, transition->target);

    self->active_state = hsm4c_walk_down_to_leaf_with_initial_and_enter(self, transition->target);

    // Repeat transition search for completion event
    transition =
        hsm4c_find_transition(self, (hsm4c_trigger_t){.variant = HSM4C_TRIGGER_COMPLETION_EVENT});

    if (!transition) {
      HSM4C_LOG("%s: no (more) completion transitions\n", __func__);
      return HSM4C_OK;
    }
  }

  return HSM4C_OK;
}

hsm4c_transition_t const *hsm4c_find_transition(hsm4c_t const *self, hsm4c_trigger_t trigger) {
  HSM4C_ASSERT(self->cfg->states[self->active_state].variant == HSM4C_STATE_COMPOUND);

  // Iterate over the states from the current state to the root state.
  for (hsm4c_state_id_t state_id = self->active_state; state_id != HSM4C_STATE_ID_ZERO;
       hsm4c_cursor_up(self, &state_id)) {
    // Iterate over the transitions of the current state and find the first transition that matches
    hsm4c_transition_t const *const match =
        hsm4c_find_first_matching_transition(self, state_id, trigger);
    if (match) {
      return match;
    }
  }

  return NULL;
}

hsm4c_state_id_t hsm4c_get_parent_state_id(hsm4c_t const *self, hsm4c_state_id_t state_id) {
  return self->cfg->states[state_id].parent;
}

void hsm4c_print_current_state_branch(hsm4c_t const *self) {
  hsm4c_state_id_t cursor = self->active_state;

  while (1) {
    HSM4C_PRINT("%s <- ", hsm4c_get_name_or_addr(self, cursor));
    hsm4c_cursor_up(self, &cursor);
    if (cursor == HSM4C_STATE_ID_ZERO) {
      HSM4C_PRINT("%s\n", hsm4c_get_name_or_addr(self, cursor));
      break;
    }
  }
}

void hsm4c_print_transition(hsm4c_t const *self, hsm4c_transition_t const *transition) {
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
  hsm4c_state_id_t const source_state = transition->source;
#else
  hsm4c_state_id_t const source_state = self->active_state;
#endif

  HSM4C_PRINT("%s: %s -> %s\n", __func__, hsm4c_get_name_or_addr(self, source_state),
              hsm4c_get_name_or_addr(self, transition->target));
}
