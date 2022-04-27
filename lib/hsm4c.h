#include <stddef.h>



typedef struct state_s state_t;
typedef struct transition_s transition_t;

typedef void (*action_fn_p)(void);

typedef enum {
    NONE,
    EVENT_A,
    EVENT_B,
} event_t;

struct transition_s {
    action_fn_p action;
    event_t event;
    state_t *next;
};

struct state_s {
    action_fn_p entry;
    action_fn_p exit;
    transition_t *transitions;
    size_t num_transitions;
};

struct statem_s {
    state_t *states;
};

void dispatch(event_t event);