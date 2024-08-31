#pragma once

#include "defines.h"
#include "platform/platform.h"

#define MAX_INPUTACTION_NAME_LENGTH 64
#define MAX_INPUTACTION_BINDINGS 8

struct InputDevice;

typedef bool (*InputKeyDown)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyUp)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyWasDown)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyWasUp)(u32 sub_id, u32 key_code);
typedef f32 (*InputKeyGetCurrentValue)(u32 sub_id, u32 key_code);
typedef f32 (*InputKeyGetPreviousValue)(u32 sub_id, u32 key_code);
typedef void (*InputProcessKey)(u32 sub_id, u32 key_code, bool is_down);
typedef void* (*InputGetCurrentState)(u32 sub_id);
typedef void* (*InputGetPreviousState)(u32 sub_id);
typedef bool (*InputIsConnected)(u32 sub_id);
typedef bool (*InputOnConnected)(u32 sub_id, struct InputDevice* device);
typedef bool (*InputOnDisconnected)(u32 sub_id);

typedef enum DeviceType
{
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_GAMEPAD,
} DeviceType;

typedef enum DeviceGamepadType
{
    DEVICE_TYPE_GAMEPAD_NONE,
    DEVICE_TYPE_GAMEPAD_XBOX,
    DEVICE_TYPE_GAMEPAD_DUALSHOCK4,
    DEVICE_TYPE_GAMEPAD_SWITCH,
    DEVICE_TYPE_GAMEPAD_STEAM,
    DEVICE_TYPE_GAMEPAD_GENERIC
} DeviceGamepadType;

typedef struct InputDevice 
{
    u32 id;
    u32 sub_id;

    PlatformHIDDevice hid_device;
    
    InputKeyDown key_down;
    InputKeyUp key_up;
    InputKeyWasDown key_was_down;
    InputKeyWasUp key_was_up;
    
    InputKeyGetCurrentValue get_current_key_value;
    InputKeyGetPreviousValue get_previous_key_value;

    InputProcessKey process_key;

    InputGetCurrentState get_current_state;
    InputGetPreviousState get_previous_state;

    InputIsConnected is_connected;
    InputOnConnected on_connected;
    InputOnDisconnected on_disconnected;
    
    u64 state_size;
} InputDevice;

#define DEVICE_VALID(device) (device.id != 0 && device.key_down && device.key_up && device.key_was_down && device.key_was_up)
#define IS_SAME_DEVICE(device, dev_id, sub_id) ((device).id == (dev_id) && (device).sub_id == (sub_id))
#define DEVICE_SUB_ID_ANY -1

typedef struct InputMapping 
{
    u32 device_id;
    i32 sub_id;
    u32 key_code;
    bool inverted;
    f32 deadzone;
} InputMapping;

typedef enum InputActionAxisType
{
    INPUT_ACTION_AXIS_TYPE_NONE = 0,
    INPUT_ACTION_AXIS_TYPE_NATIVE,
    INPUT_ACTION_AXIS_TYPE_VIRTUAL,
} InputActionAxisType;

typedef struct InputActionBinding
{
    InputActionAxisType axis_type;
    InputMapping mapping0;
    InputMapping mapping1;
} InputActionBinding;

typedef enum InputActionType
{
    INPUT_ACTION_TYPE_NONE = 0,
    INPUT_ACTION_TYPE_BUTTON,
    INPUT_ACTION_TYPE_AXIS
} InputActionType;

typedef struct InputAction
{
    char name[MAX_INPUTACTION_NAME_LENGTH];
    InputActionType type;
    u32 bindings_count;
    InputActionBinding bindings[MAX_INPUTACTION_BINDINGS];
} InputAction;