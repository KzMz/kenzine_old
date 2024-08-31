#pragma once

#include "defines.h"
#include "core/input/input_defines.h"
#include "core/input/devices/input_gamepad.h"

// XBOX Gamepad
bool platform_gamepad_xbox_set_vibration(u32 sub_id, GamepadVibration vibration);
void* platform_gamepad_xbox_get_current_state(u32 sub_id);

bool platform_gamepad_xbox_button_down(u32 sub_id, u32 button);
bool platform_gamepad_xbox_button_up(u32 sub_id, u32 button);
bool platform_gamepad_xbox_button_was_down(u32 sub_id, u32 button);
bool platform_gamepad_xbox_button_was_up(u32 sub_id, u32 button);

f32 platform_gamepad_xbox_scalar_current_value(u32 sub_id, u32 button);
f32 platform_gamepad_xbox_scalar_previous_value(u32 sub_id, u32 button);

