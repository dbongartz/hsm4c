#include "hsm4c.h"

#if HSM4C_CONFIG_LOG
  #ifndef HSM4C_LOG
    #include <stdio.h>
    #define HSM4C_LOG(...)                                                                         \
      do {                                                                                         \
        if (HSM4C_CONFIG_LOG)                                                                      \
          fprintf(stderr, __VA_ARGS__);                                                            \
      } while (0)
  #endif
#else
  #define HSM4C_LOG(...)                                                                           \
    do {                                                                                           \
    } while (false)
#endif

static hsm4c_state_id_t hsm4c_cursor_down(hsm4c_t const *self, hsm4c_state_id_t cursor) {
  assert(self->cfg->states[cursor].variant == HSM4C_STATE_COMPOUND);
  return self->cfg->states_rt[cursor].compound.active_substate;
}

static hsm4c_state_id_t hsm4c_cursor_up(hsm4c_t const *self, hsm4c_state_id_t cursor) {
  return self->cfg->states[cursor].parent;
}

static void hsm4c_initialize_active_substate(hsm4c_t const *self, hsm4c_state_id_t cursor) {
  assert(self->cfg->states[cursor].variant == HSM4C_STATE_COMPOUND);
  self->cfg->states_rt[cursor].compound.active_substate =
      self->cfg->states[cursor].compound.initial;
}

hsm4c_result_e hsm4c_init(hsm4c_t *self, hsm4c_cfg_t const *cfg) {
  self->cfg = cfg;
  self->current_state = HSM4C_STATE_ID_RESERVED;

  hsm4c_state_t const *s = self->cfg->states;
  hsm4c_state_id_t cursor = self->current_state;

  while (1) {
    assert(s[cursor].variant == HSM4C_STATE_COMPOUND);

#if HSM4C_CONFIG_ENTRY_FN
    if (s[cursor].compound.entry_fn) {
      s[cursor].compound.entry_fn(cfg->ctx, &s[cursor]);
    }
#endif

    hsm4c_initialize_active_substate(self, cursor);
    self->current_state = cursor;
    cursor = hsm4c_cursor_down(self, cursor);

    if (cursor == HSM4C_STATE_ID_RESERVED) {
      break;
    }
  }

  return HSM4C_OK;
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

static bool hsm4c_has_guard(hsm4c_transition_t const *transition) {
#if HSM4C_CONFIG_TRANSITION_GUARDS
  return transition->guard_fn;
#else
  (void)transition;
  return false;
#endif
}

static bool hsm4c_check_guard(hsm4c_t const *self, hsm4c_transition_t const *transition,
                              hsm4c_trigger_t trigger) {
#if HSM4C_CONFIG_TRANSITION_GUARDS
  if (hsm4c_has_guard(transition)) {
    return transition->guard_fn(self->cfg->ctx, trigger);
  } else {
    return true;
  }
#else
  (void)self;
  (void)transition;
  (void)trigger;
  return true;
#endif
}

static hsm4c_transition_t const *hsm4c_find_transition(hsm4c_t const *self,
                                                       hsm4c_trigger_t trigger) {

  hsm4c_state_id_t cursor = self->current_state;

  assert(self->cfg->states[cursor].variant == HSM4C_STATE_COMPOUND);

  static hsm4c_trigger_t const completion_trigger = {.variant = HSM4C_TRIGGER_COMPLETION_EVENT};

  enum {
    HSM4C_MATCH_UNSPECIFIED,
    HSM4C_MATCH_TRIGGER,
    HSM4C_MATCH_GUARD_ONLY,
    HSM4C_MATCH_COMPLETION_ONLY,
  } match_level = HSM4C_MATCH_UNSPECIFIED;

  while (cursor != HSM4C_STATE_ID_RESERVED) {

#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
    hsm4c_transition_t const *const ts = self->cfg->transitions;
    size_t const transitions_num = self->cfg->transitions_num;
#else
    hsm4c_transition_t const *const ts = self->cfg->states[cursor].compound.transitions;
    hsm4c_size_t const transitions_num = self->cfg->states[cursor].compound.transitions_num;
#endif

    size_t match_completion_only_id = 0;
    size_t match_guard_only_id = 0;
    size_t match_trigger_id = 0;

    for (size_t i = 0; i < transitions_num; ++i) {
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
      if (ts[i].source != cursor) {
        continue;
      }
#endif

      if (hsm4c_trigger_eq(ts[i].trigger, completion_trigger) && !hsm4c_has_guard(&ts[i])) {
        match_level = HSM4C_MATCH_COMPLETION_ONLY;
        match_completion_only_id = i;
        break;
      }

      if (match_level < HSM4C_MATCH_GUARD_ONLY &&
          hsm4c_trigger_eq(ts[i].trigger, completion_trigger) && hsm4c_has_guard(&ts[i]) &&
          hsm4c_check_guard(self, &ts[i], trigger)) {
        match_level = HSM4C_MATCH_GUARD_ONLY;
        match_guard_only_id = i;
      }

      if (match_level < HSM4C_MATCH_TRIGGER && hsm4c_trigger_eq(ts[i].trigger, trigger) &&
          (!hsm4c_has_guard(&ts[i]) || hsm4c_check_guard(self, &ts[i], trigger))) {
        match_level = HSM4C_MATCH_TRIGGER;
        match_trigger_id = i;
      }
    }

    if (match_level == HSM4C_MATCH_COMPLETION_ONLY) {
      HSM4C_LOG("transition: completion: %zu\n", match_completion_only_id);
      return &ts[match_completion_only_id];
    }

    if (match_level == HSM4C_MATCH_GUARD_ONLY) {
      HSM4C_LOG("transition: completion guard: %zu\n", match_guard_only_id);
      return &ts[match_guard_only_id];
    }

    if (match_level == HSM4C_MATCH_TRIGGER) {
      HSM4C_LOG("transition: specific: %zu\n", match_trigger_id);
      return &ts[match_trigger_id];
    }

    cursor = hsm4c_cursor_up(self, cursor);
  }
  return NULL;
}

#if HSM4C_CONFIG_STATE_NAME
static char const *hsm4c_get_name_or_addr(hsm4c_t const *self, hsm4c_state_id_t id) {
  return self->cfg->states[id].name;
}
#else
static char const *hsm4c_get_name_or_addr(hsm4c_t const *self, hsm4c_state_id_t id) {
  (void)self;
  static char s[12];
  snprintf(s, 12, "%p", (void *)&self->cfg->states[id]);
  return s;
}
#endif

hsm4c_result_e hsm4c_run2completion(hsm4c_t *self, hsm4c_trigger_t trigger) {
  hsm4c_transition_t const *t = hsm4c_find_transition(self, trigger);
  if (t) {
#if HSM4C_CONFIG_UNIFIED_TRANSITION_TABLE
    HSM4C_LOG("transition: s: %s, t: %s\n", hsm4c_get_name_or_addr(self, t->source),
              hsm4c_get_name_or_addr(self, t->target));
#else
    HSM4C_LOG("transition: s: %s, t: %s\n", hsm4c_get_name_or_addr(self, self->current_state),
              hsm4c_get_name_or_addr(self, t->target));
#endif
  } else {
    HSM4C_LOG("transition: not found\n");
  }
  return HSM4C_OK;
}

void hsm4c_print_states(hsm4c_t const *self) {
  hsm4c_state_id_t cursor = self->current_state;

  while (1) {
    HSM4C_LOG("%s<-", hsm4c_get_name_or_addr(self, cursor));
    cursor = hsm4c_cursor_up(self, cursor);
    if (cursor == HSM4C_STATE_ID_RESERVED) {
      HSM4C_LOG("%s\n", hsm4c_get_name_or_addr(self, cursor));
      break;
    }
  }
}
