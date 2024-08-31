#pragma once

#include "defines.h"
#include "core/input/input_defines.h"
#include "lib/math/math_defines.h"

#define GAMEPAD_DEVICE_ID 3

// TODO: deadzones
#define GAMEPAD_TRIGGER_THRESHOLD 30
#define GAMEPAD_LEFT_THUMB_DEADZONE 7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689

typedef enum GamepadButton
{
    GAMEPAD_BUTTON_FACE_TOP = 0,
    GAMEPAD_BUTTON_FACE_RIGHT,
    GAMEPAD_BUTTON_FACE_BOTTOM,
    GAMEPAD_BUTTON_FACE_LEFT,
    GAMEPAD_BUTTON_SHOULDER_LEFT,
    GAMEPAD_BUTTON_SHOULDER_RIGHT,
    GAMEPAD_BUTTON_THUMB_LEFT,
    GAMEPAD_BUTTON_THUMB_RIGHT,
    GAMEPAD_BUTTON_DPAD_UP,
    GAMEPAD_BUTTON_DPAD_RIGHT,
    GAMEPAD_BUTTON_DPAD_DOWN,
    GAMEPAD_BUTTON_DPAD_LEFT,
    GAMEPAD_BUTTON_START,
    GAMEPAD_BUTTON_BACK,
    GAMEPAD_BUTTON_COUNT
} GamepadButton;

typedef enum GamepadAxis
{
    GAMEPAD_AXIS_LEFT_THUMB_X = 0,
    GAMEPAD_AXIS_LEFT_THUMB_Y,
    GAMEPAD_AXIS_RIGHT_THUMB_X,
    GAMEPAD_AXIS_RIGHT_THUMB_Y,
    GAMEPAD_AXIS_TRIGGER_LEFT,
    GAMEPAD_AXIS_TRIGGER_RIGHT,
    GAMEPAD_AXIS_COUNT
} GamepadAxis;

typedef struct GamepadVibration
{
    f32 left_motor;
    f32 right_motor;
} GamepadVibration;

typedef struct GamepadState
{
    bool buttons[GAMEPAD_BUTTON_COUNT];
    f32 axes[GAMEPAD_AXIS_COUNT];
    GamepadVibration vibration;
    bool connected; 
} GamepadState;

typedef bool (*GamepadSetVibration)(u32 sub_id, GamepadVibration vibration);

u32 gamepad_register(u32 sub_id, u32 type);
void gamepad_unregister(u32 sub_id);
void* gamepad_get_current_state(u32 sub_id);

KENZINE_API bool gamepad_is_connected(u32 sub_id);

KENZINE_API GamepadVibration gamepad_get_vibration(u32 sub_id);
KENZINE_API void gamepad_set_vibration(u32 sub_id, GamepadVibration vibration);