#include "input_gamepad.h"
#include "core/input/input.h"
#include "input_gamepad_platform.h"
#include <stddef.h>

typedef struct GamepadDevice
{
    u32 internal_id;
    DeviceGamepadType type;
    GamepadState current_state;
    GamepadState previous_state;

    GamepadSetVibration set_vibration;
} GamepadDevice;

// TODO: make this configurable
#define MAX_GAMEPADS 4
static GamepadDevice gamepads[MAX_GAMEPADS]; // indexed by sub_id

void* gamepad_get_previous_state(u32 sub_id);

bool gamepad_on_connected(u32 sub_id, InputDevice* device);
bool gamepad_on_disconnected(u32 sub_id);

bool gamepad_button_down(u32 sub_id, u32 button)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return false;
    }

    return gamepads[sub_id].current_state.buttons[button];
}

bool gamepad_button_up(u32 sub_id, u32 button)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return false;
    }

    return !gamepads[sub_id].current_state.buttons[button];
}

bool gamepad_button_was_down(u32 sub_id, u32 button)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return false;
    }

    return gamepads[sub_id].previous_state.buttons[button];
}

bool gamepad_button_was_up(u32 sub_id, u32 button)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return false;
    }

    return !gamepads[sub_id].previous_state.buttons[button];
}

f32 gamepad_axis_current_value(u32 sub_id, u32 axis)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return 0.0f;
    }

    return gamepads[sub_id].current_state.axes[axis];
}

f32 gamepad_axis_previous_value(u32 sub_id, u32 axis)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return 0.0f;
    }

    return gamepads[sub_id].previous_state.axes[axis];
}

u32 gamepad_register(u32 sub_id, u32 type)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return INVALID_ID;
    }

    gamepads[sub_id] = (GamepadDevice) {0};
    InputDevice device = {0};

    switch (type)
    {
        case DEVICE_TYPE_GAMEPAD_XBOX:
        {
            gamepads[sub_id].type = DEVICE_TYPE_GAMEPAD_XBOX;
            gamepads[sub_id].set_vibration = platform_gamepad_xbox_set_vibration;
            device = (InputDevice) 
            {
                .id = GAMEPAD_DEVICE_ID,
                .sub_id = sub_id,
                .key_down = gamepad_button_down,
                .key_up = gamepad_button_up,
                .key_was_down = gamepad_button_was_down,
                .key_was_up = gamepad_button_was_up,
                .get_current_key_value = gamepad_axis_current_value,
                .get_previous_key_value = gamepad_axis_previous_value,
                .get_current_state = platform_gamepad_xbox_get_current_state,
                .get_previous_state = gamepad_get_previous_state,
                .is_connected = gamepad_is_connected,
                .on_connected = gamepad_on_connected,
                .on_disconnected = gamepad_on_disconnected,
                .process_key = NULL,
                .state_size = sizeof(GamepadState),
            };
        } break;
    }
    
    u32 index = input_register_device(device);
    return index;
}

void gamepad_unregister(u32 sub_id)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return;
    }

    gamepads[sub_id] = (GamepadDevice) {0};
    gamepads[sub_id].internal_id = INVALID_ID;
}

void* gamepad_get_current_state(u32 sub_id)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return NULL;
    }

    return &gamepads[sub_id].current_state;
}

void* gamepad_get_previous_state(u32 sub_id)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return NULL;
    }

    return &gamepads[sub_id].previous_state;
}

bool gamepad_is_connected(u32 sub_id)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return false;
    }

    return gamepads[sub_id].current_state.connected;
}

bool gamepad_on_connected(u32 sub_id, InputDevice* device)
{
    return true;
}

bool gamepad_on_disconnected(u32 sub_id)
{
    gamepad_unregister(sub_id);
    return true;
}

GamepadVibration gamepad_get_vibration(u32 sub_id)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return (GamepadVibration) {0};
    }

    return gamepads[sub_id].current_state.vibration;
}

void gamepad_set_vibration(u32 sub_id, GamepadVibration vibration)
{
    if (sub_id >= MAX_GAMEPADS)
    {
        return;
    }

    if (gamepads[sub_id].set_vibration)
    {
        gamepads[sub_id].set_vibration(sub_id, vibration);
    }
    
    gamepads[sub_id].current_state.vibration = vibration;
}