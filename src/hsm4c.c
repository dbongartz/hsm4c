#include "hsm4c.h"

#include <stdio.h>

#if HSM4C_CONFIG_LOG
  #define HSM4C_LOG(...) printf(__VA_ARGS__)
#else
  #define HSM4C_LOG(...)                                                                           \
    do {                                                                                           \
    } while (false)
#endif

#if HSM4C_CONFIG_ASSERT
  #include "assert.h"
  #define HSM4C_ASSERT(...) assert(__VA_ARGS__)
#endif

static void state_entry(hsm4c_state_t *state) {
#if HSM4C_CONFIG_ENTRY_FN
  if (state->cfg->entry_fn) {
    HSM4C_LOG("entry_fn: %s\n", hsm4c_get_name(state));
    state->cfg->entry_fn(state);
  }
#endif
  (void)state;
}

static void state_exit(hsm4c_state_t *state) {
#if HSM4C_CONFIG_EXIT_FN
  if (state->cfg->exit_fn) {
    HSM4C_LOG("exit_fn: %s\n", hsm4c_get_name(state));
    state->cfg->exit_fn(state);
  }
#endif
  (void)state;
}

static bool check_guard(hsm4c_transition_t const *transition, hsm4c_event_t event) {
#if HSM4C_CONFIG_TRANSITION_GUARDS
  if (transition->guard_fn) {
    HSM4C_LOG("guard_fn: e: %d\n", event.id);
    return transition->guard_fn(event);
  }
#endif
  (void)event;
  (void)transition;
  return true;
}

static void transition_fn(hsm4c_transition_t const *transition, hsm4c_event_t event) {
#if HSM4C_CONFIG_TRANSITION_FN
  if (transition->transition_fn) {
    HSM4C_LOG("transition_fn: e: %d\n", event.id);
    transition->transition_fn(event);
  }
#endif
  (void)transition;
  (void)event;
}

#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
static bool is_valid_internal_transition(hsm4c_state_t const *state, hsm4c_target_state_t target) {
  // FIXME: Internal transitions should work when target:
  // - Same state -> no action
  // - Any parent & grandparent states -> no entry on target
  // - Any child & grandchildren -> no exit on source

  return state == target.state;
}
#endif

#if HSM4C_CONFIG_HIERARCHICAL
static void set_active_child(hsm4c_state_t *state, hsm4c_state_t *child) {
  if (state) {
    state->active_child = child;
  }
}
#endif

#if HSM4C_CONFIG_HISTORY_STATES
static hsm4c_state_t *entry_history(hsm4c_state_t *target, hsm4c_history_e history) {
  hsm4c_state_t *leaf = target;

  // If in deep history, just continue down the active child path.
  // Fallback to inital state if no active child.
  if (history == HSM4C_HISTORY_DEEP) {
    hsm4c_state_t *s = leaf;
    while (s != NULL) {
      state_entry(s);
      leaf = s;
      s = s->active_child ? s->active_child : s->cfg->initial;
    }
    return leaf;
  }

  // For shall history we skip resetting the active_child of the target only.
  if (history == HSM4C_HISTORY_SHALLOW) {
    state_entry(leaf);
    leaf = target->active_child ? target->active_child : target->cfg->initial;
  }

  return leaf;
}
#endif

#if HSM4C_CONFIG_HIERARCHICAL
static hsm4c_state_t *entry_branch(hsm4c_state_t *root, hsm4c_target_state_t target) {
  hsm4c_state_t *leaf = target.state;

  // Initialize active child until target
  for (hsm4c_state_t *s = target.state; s != root; s = s->cfg->parent) {
    set_active_child(target.state->cfg->parent, target.state);
  }

  // First, enter till target, already initialized
  for (leaf = root; leaf != target.state; leaf = leaf->active_child) {
    state_entry(leaf);
  }

  #if HSM4C_CONFIG_HISTORY_STATES
  entry_history(leaf, target.history);
  #endif

  // Enter & init rest
  for (hsm4c_state_t *s = leaf; s != NULL; s = s->cfg->initial) {
    s->active_child = s->cfg->initial;
    set_active_child(s, s->cfg->initial);
    state_entry(s);
    leaf = s;
  }

  // leaf might be NULL if the target state has no children.
  return leaf ? leaf : target.state;
}
#endif

#if HSM4C_CONFIG_HIERARCHICAL
static hsm4c_state_t *external_transition(hsm4c_transition_t const *transition,
                                          hsm4c_state_t *state, hsm4c_target_state_t target,
                                          hsm4c_event_t event) {
  hsm4c_state_t *common_ancestor = NULL;
  hsm4c_state_t *source_branch_ancestor = NULL;
  hsm4c_state_t *target_branch_ancestor = NULL;

  // FIXME: Internal transitions should work when target:
  // - Same state -> no action
  // - Any parent & grandparent states -> no entry on target
  // - Any child & grandchildren -> no exit on source

  for (hsm4c_state_t *source_node = state; source_node; source_node = source_node->cfg->parent) {
    source_branch_ancestor = source_node;

    if (target.transition_type == HSM4C_TRANSITION_EXTERNAL || source_node != state) {
      state_exit(source_node);
    }

    for (hsm4c_state_t *target_node = target.state; target_node;
         target_node = target_node->cfg->parent) {
      target_branch_ancestor = target_node;
      if (source_node == target_node) {
        common_ancestor = source_node;
        break;
      }
    }
  }

  HSM4C_LOG("ca: %s, src_a: %s, trg_a: %s\n",
            common_ancestor ? hsm4c_get_name(common_ancestor) : "NULL",
            hsm4c_get_name(source_branch_ancestor), hsm4c_get_name(target_branch_ancestor));

  transition_fn(transition, event);

  return entry_branch(target_branch_ancestor, target);
}
#else  // HSM4C_CONFIG_HIERARCHICAL
static hsm4c_state_t *external_transition(hsm4c_transition_t const *transition,
                                          hsm4c_state_t *state, hsm4c_target_state_t target,
                                          hsm4c_event_t event) {
  state_exit(state);
  transition_fn(transition, event);
  state_entry(target.state);
  return target.state;
}
#endif // HSM4C_CONFIG_HIERARCHICAL

#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
  #if HSM4C_CONFIG_HIERARCHICAL
static hsm4c_state_t *internal_transition(hsm4c_transition_t const *transition,
                                          hsm4c_state_t *state, hsm4c_state_t *target,
                                          hsm4c_event_t event) {
  if (state->cfg->parent == target) {
    state_exit(state);
  }

  transition_fn(transition, event);

  if (target->cfg->parent == state) {
    state_entry(target);
  }

  return target;
}
  #else  // HSM4C_CONFIG_HIERARCHICAL
static hsm4c_state_t *internal_transition(hsm4c_transition_t const *transition,
                                          hsm4c_state_t *state, hsm4c_state_t *target,
                                          hsm4c_event_t event) {
  (void)state;
  transition_fn(transition, event);
  return target;
}
  #endif // HSM4C_CONFIG_HIERARCHICAL
#endif   // HSM4C_CONFIG_INTERNAL_TRANSITIONS

static hsm4c_state_t *do_transition(hsm4c_transition_t const *transition, hsm4c_state_t *state,
                                    hsm4c_target_state_t target, hsm4c_event_t event) {
#if HSM4C_CONFIG_INTERNAL_TRANSITIONS
  if (target.transition_type == HSM4C_TRANSITION_INTERNAL &&
      is_valid_internal_transition(state, target)) {
    HSM4C_LOG("Internal transition\n");
    return internal_transition(transition, state, target.state, event);
  } else
#endif
  {
    HSM4C_LOG("External transition\n");
    return external_transition(transition, state, target, event);
  }
}

static hsm4c_target_state_t get_target(hsm4c_transition_t const *transition, hsm4c_state_t *state,
                                       hsm4c_event_t event) {
  (void)event;

  hsm4c_target_state_t target = transition->target.fixed;

#if HSM4C_CONFIG_TARGET_CHOICE
  if (transition->target_type == HSM4C_TARGET_CHOICE_FN) {
    target = transition->target.choice_fn(event);
  }
#endif

  // If state_next is NULL, it means that the transition is a self-transition.
  target.state = target.state ? target.state : state;

  return target;
}

/* -------- PUBLIC -------- */

hsm4c_state_t *hsm4c_init(hsm4c_state_t *initial) {
#if HSM4C_CONFIG_HIERARCHICAL
  hsm4c_state_t *root = NULL;
  for (hsm4c_state_t *s = initial; s; s = s->cfg->parent) {
    s->active_child = s;
    root = s;
  }

  hsm4c_state_t *leaf = NULL;
  for (hsm4c_state_t *s = root; s != NULL; s = s->cfg->initial) {
    s->active_child = s->cfg->initial;
    state_entry(s);
    leaf = s;
  }
  HSM4C_LOG("initial: %s, root: %s, leaf: %s\n", hsm4c_get_name(initial), hsm4c_get_name(root),
            hsm4c_get_name(leaf));
  return leaf ? leaf : initial;
#else
  state_entry(initial);
  HSM4C_LOG("initial: %s", hsm4c_get_name(initial));
  return initial;
#endif
}

hsm4c_state_t *hsm4c_dispatch(hsm4c_state_t *state, hsm4c_event_t event) {
  hsm4c_state_cfg_t const *cfg = state->cfg;

  for (size_t i = 0; i < cfg->num_transitions; ++i) {
    hsm4c_transition_t const *transition = &cfg->transitions[i];
    if (transition->event_id == event.id && check_guard(transition, event)) {
      hsm4c_target_state_t target = get_target(transition, state, event);
      do_transition(transition, state, target, event);
      return target.state;
    }
  }

  return state;
}

char const *hsm4c_get_name(hsm4c_state_t const *state) {
#if HSM4C_CONFIG_NAME
  return state->cfg->name ? state->cfg->name : "";
#endif
  (void)state;
  return "";
}
