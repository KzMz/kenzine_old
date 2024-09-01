#pragma once

#include "defines.h"
#include "core/input/input_defines.h"
#include "core/input/devices/input_gamepad.h"

// XBOX Gamepad
bool platform_gamepad_xbox_set_vibration(u32 sub_id, GamepadVibration vibration);
void* platform_gamepad_xbox_get_current_state(u32 sub_id);

// TODO: to be defined
// https://github.com/MysteriousJ/Joystick-Input-Examples/blob/main/src/combined.cpp
// Generic gamepads, ps4 gamepads, switch gamepads, steam input api
// Do we need input buffer? https://github.com/MysteriousJ/Joystick-Input-Examples/blob/main/src/rawinput_buffered.cpp
// https://github.com/libusb/hidapi/blob/master/windows/hid.c
// https://github.com/weigert/joycon-/blob/master/joycon-core/joycon-core.h