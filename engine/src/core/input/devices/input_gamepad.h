#pragma once

#include "defines.h"

bool mouse_button_down(u32 button);
bool mouse_button_up(u32 button);
bool mouse_button_was_down(u32 button);
bool mouse_button_was_up(u32 button);

f32 mouse_button_current_value(u32 button);
f32 mouse_button_previous_value(u32 button);

void mouse_process_button(u32 button, bool is_down);

void gamepad_register(u32 id, u32 type);
void gamepad_unregister(u32 id);

void* gamepad_get_current_state(u32 id);
void* gamepad_get_previous_state(u32 id);