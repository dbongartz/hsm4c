#pragma once

#include "../lib/hsm4c.h"

void s_entry(State const *s);
void s_exit(State const *s);
State *s_run(State const *s, EventType event);

bool t_guard(State const *s);
void t_action(State const *s);
