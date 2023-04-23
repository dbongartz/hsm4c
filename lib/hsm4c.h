/**
 * \brief Statechart implementation in C
 * \file
 *
 * Tries to make UML2 statecharts accessible in C.
 *
 * Supports:
 *
 * - Fully hierachical statecharts.
 * - Entry/Exit/Run state actions.
 * - Run can trigger immediate transition.
 * - Transitions with guard and action.
 * - Internal (Local) and external transitions.
 * - Automatic transitions.
 * - Initial child states.
 * - History and Deep History pseudo states with initial state.
 * - Choice pseudo states.
 * - Relatively easy table based syntax. (See tests).
 *
 * Does not support:
 * - Parallel states
 *
 * (C) 2023 David Bongartz
 * MIT License
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct State State;
typedef struct StateConfig StateConfig;
typedef struct Transition Transition;
typedef int EventType;

/** \brief State Types */
typedef enum StateType {
  /** \brief Default (compund) state type */
  SC_TYPE_NORMAL = 0,

  /**
   * \brief History pseudo state. Must not have entry/exit/run/children.
   *
   * Parent states last active substate will be entered and reset.
   * If parent does not have an active substate, will try using the
   * specified initial state if present, the parents initial state otherwise.
   */
  SC_TYPE_HISTORY,

  /**
   * \brief Deep History pseudo state. Must not have entry/exit/run/children.
   *
   * Parent states last active substate and children of that one will be entered
   * and NOT reset.
   *
   * If parent does not have an active substate, will try using the
   * specified initial state if present, the parents initial state otherwise.
   */
  SC_TYPE_HISTORY_DEEP,

  /**
   * \brief Choice pseudo state.
   *
   * Must not have entry/exit/run/children.
   * Must only have automatic (SC_NO_EVENT) guarded transition. Transitions are evaluated in
   * table order.
   */
  SC_TYPE_CHOICE,

  /**
   * \brief Root pseudo state.
   *
   * Can have entry/exit/run functions.
   * Must have initial state.
   * Must have no parent (NULL).
   */
  SC_TYPE_ROOT,
} StateType;

/** \brief Transition Types */
typedef enum TransitionType {
  /** \brief External (default) transition. Source and target states are exited and entered. */
  SC_TTYPE_EXTERNAL = 0,

  /**
   * \brief Local transition. Source is not exited.
   *
   * Must be "Parent -> Child" or "Same -> Same"
   * When "Same -> Same" no entry and exit function is executed (internal transition)
   */
  SC_TTYPE_LOCAL,

  /**
   * \brief Use to indicate the end of transition tables
   */
  SC_TTYPE_TABLE_END,
} TransitionType;

/** \brief Special events. Must be <= 0 */
typedef enum ScEvents {
  /** \brief Use this for event-less / automatic transitions. */
  SC_NO_EVENT = 0,
} ScEvents;

/**
 * \brief Entry function prototype.
 *
 * \param s   Current state
 */
typedef void (*entry_fn)(State const *s);

/**
 * \brief Run function prototype.
 *
 * \param s   Current state.
 * \param e   Event of transition if any. SC_NO_EVENT if not in transition.
 *
 * \return    Valid state to trigger immediate transition. NULL to not change state.
 *
 * \attention Transition in run are not visible in the transition table.
 *            Be careful with this.
 */
typedef State *(*run_fn)(State const *s, EventType e);

/**
 * \brief Exit function prototype.
 *
 * \param s   Current state
 */
typedef void (*exit_fn)(State const *s);

/**
 * \brief Transition action prototype.
 *
 * \param s   Root state
 */
typedef void (*transition_fn)(State const *root);

/**
 * \brief Guard prototype.
 *
 * \param s   Root state
 *
 * \return    true if transition should be taken. false otherwise.
 */
typedef bool (*guard_fn)(State const *root);

/** \brief Transition class */
struct Transition {
  /** \brief Source state of transition. Must be a valid state. */
  State *const from;
  /** \brief Target state of transition. Must be a valid state. */
  State *const to;
  /** \brief Event the transition reacts to. Must be positive or one of ScEvents */
  EventType const event;
  /** \brief Transition function. Will be called after all exits, before all entrys */
  transition_fn const transition_fn;
  /** \brief Guard. Return true to take transition. Called when matching source and event found */
  guard_fn const guard_fn;
  /** \brief Transition type. Default: External */
  TransitionType type;
};

/** \brief Use this to indicate the end of the transition table. */
#define SC_TRANSITIONS_END ((Transition const){.type = SC_TTYPE_TABLE_END})

struct StateConfig {
  /** \brief Name of the state (optional) */
  char const *name;
  /** \brief Entry function. Will be called after transition guard. (optional) */
  entry_fn const entry_fn;
  /** \brief Run function. Can change state. After transition or when no transition (optional) */
  run_fn const run_fn;
  /** \brief Entry function. After transition guard. (optional) */
  exit_fn const exit_fn;
  /** \brief Parent state. Must be statchart root or NULL if this is root state. (mandatory) */
  State *const parent;
  /** \brief Initial state. When target is this state, also transition into initial. (optional) */
  State *const initial;
  /** \brief State type. See StateType description. (optional) */
  StateType type;
    /** \brief State transition table. Only used on root node currently. */
  Transition const *transitions;
};

/** \brief State class */
struct State {
  StateConfig const *config;

  /** \brief Active child state. On root node this is always a leaf */
  State *_active;

};

/**
 * \brief Initialized a statechart
 *
 * This does only initialize the root tree.
 * To reset all states please iterate with `sc_reset_state()`.
 *
 * \param root          Statechart root state.
 * \param transitions   Transition table. Last element must be SC_TRANSITIONS_END.
 *
 * \return              Leaf state after init.
 */
State const *sc_init(State *root);

/** \brief Resets the given state */
void sc_reset_state(State *state);

/** \brief Map StateConfigs and State if using tables to define them */
void sc_map_stateconfig_to_states(size_t num_states, State states[num_states],
                                  StateConfig const statecfgs[num_states]);

/**
 * \brief Runs one iteration of the statechart
 *
 * \param root    Statechart root state.
 * \param event   Event to pass to the statechart.
 *                Events <= 0 are used internally.
 *                E.g. SC_NO_EVENT is 0
 *
 * \return        State after one iteration.
 */
State const *sc_run(State *root, EventType event);

/**
 * \brief Get the root of any state
 *
 * \param s       State to search from.
 *
 * \return        Root state.
 */
State const *sc_get_root(State const *s);
