#include "hsm4c.h"

#include <stdio.h>

static void state_entry(hsm4c_state_t *state) {
  if (HSM4C_CONFIG_ENTRY_FN && state->cfg->entry_fn) {
    printf("entry_fn: %s\n", state->cfg->name);
    state->cfg->entry_fn(state);
  }
}

static void state_exit(hsm4c_state_t *state) {
  if (HSM4C_CONFIG_EXIT_FN && state->cfg->exit_fn) {
    printf("exit_fn: %s\n", state->cfg->name);
    state->cfg->exit_fn(state);
  }
}

static bool check_guard(hsm4c_transition_t const *transition, hsm4c_event_t event) {
  if (HSM4C_CONFIG_TRANSITION_GUARDS && transition->guard_fn) {
    printf("guard_fn: e: %d\n", event.id);
    return transition->guard_fn(event);
  }
  return true;
}

static void transition_fn(hsm4c_transition_t const *transition, hsm4c_event_t event) {
  if (HSM4C_CONFIG_TRANSITION_FN && transition->transition_fn) {
    printf("transition_fn: e: %d\n", event.id);
    transition->transition_fn(event);
  }
}

static bool is_valid_internal_transition(hsm4c_state_t const *state, hsm4c_target_state_t target) {
  return state == target.state || state->active_child == target.state ||
         state->cfg->parent == target.state;
}

static void set_child(hsm4c_state_t *state, hsm4c_state_t *child) {
  if (state) {
    state->active_child = child;
  }
}

static hsm4c_state_t *entry_branch(hsm4c_state_t *root, hsm4c_state_t *target) {
  for (hsm4c_state_t *s = root; s != target; s = s->active_child) {
    state_entry(s);
  }

  hsm4c_state_t *leaf = target;

  hsm4c_history_e history = HSM4C_HISTORY_NONE;
  if (HSM4C_CONFIG_HISTORY_STATES && history == HSM4C_HISTORY_DEEP) {
    // FIXME: Switch to setting to `initial` if `active_child` is NULL
    for (hsm4c_state_t *s = leaf; s != NULL; s = s->active_child) {
      state_entry(s);
      leaf = s;
    }
    return leaf;
  }

  if (HSM4C_CONFIG_HISTORY_STATES && history == HSM4C_HISTORY_SHALLOW) {
    state_entry(leaf);
    // FIXME: Switch to setting to `initial` if `active_child` is NULL
    leaf = target->active_child;
  }

  for (hsm4c_state_t *s = leaf; s != NULL; s = s->cfg->initial) {
    s->active_child = s->cfg->initial;
    set_child(s, s->cfg->initial);
    state_entry(s);
    leaf = s;
  }

  // leaf might be NULL if the target state has no children.
  return leaf ? leaf : target;
}

static hsm4c_state_t *external_transition(hsm4c_transition_t const *transition,
                                          hsm4c_state_t *state, hsm4c_state_t *target,
                                          hsm4c_event_t event) {
  hsm4c_state_t *common_ancestor = NULL;
  hsm4c_state_t *source_branch_ancestor = NULL;
  hsm4c_state_t *target_branch_ancestor = NULL;

  for (hsm4c_state_t *source_node = state; source_node; source_node = source_node->cfg->parent) {
    source_branch_ancestor = source_node;
    state_exit(source_node);

    for (hsm4c_state_t *target_node = target; target_node; target_node = target_node->cfg->parent) {
      target_branch_ancestor = target_node;
      set_child(target_node->cfg->parent, target_node);

      if (source_node == target_node) {
        common_ancestor = source_node;
        break;
      }
    }
  }

  printf("ca: %s, src_a: %s, trg_a: %s\n", common_ancestor ? common_ancestor->cfg->name : "NULL",
         source_branch_ancestor->cfg->name, target_branch_ancestor->cfg->name);

  transition_fn(transition, event);

  return entry_branch(target_branch_ancestor, target);
}

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

static hsm4c_state_t *do_transition(hsm4c_transition_t const *transition, hsm4c_state_t *state,
                                    hsm4c_target_state_t target, hsm4c_event_t event) {
  if (HSM4C_CONFIG_INTERNAL_TRANSITIONS && target.transition_type == HSM4C_TRANSITION_INTERNAL &&
      is_valid_internal_transition(state, target)) {
    printf("Internal transition\n");
    return internal_transition(transition, state, target.state, event);
  } else {
    printf("External transition\n");
    return external_transition(transition, state, target.state, event);
  }
}

static hsm4c_target_state_t get_target(hsm4c_transition_t const *transition, hsm4c_state_t *state,
                                       hsm4c_event_t event) {
  hsm4c_target_state_t target;
  if (HSM4C_CONFIG_TARGET_CHOICE && transition->target_type == HSM4C_TARGET_CHOICE_FN) {
    target = transition->target.choice_fn(event);
  } else {
    target = transition->target.fixed;
  }

  // If state_next is NULL, it means that the transition is a self-transition.
  target.state = target.state ? target.state : state;

  return target;
}

/* -------- PUBLIC -------- */

hsm4c_state_t *hsm4c_init(hsm4c_state_t *initial) {
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

  printf("initial: %s, root: %s, leaf: %s\n", initial->cfg->name, root->cfg->name, leaf->cfg->name);
  return initial;
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
