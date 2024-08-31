#pragma once

#include "defines.h"

#define MAX_INPUTACTION_NAME_LENGTH 64
#define MAX_INPUTACTION_BINDINGS 8

typedef bool (*InputKeyDown)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyUp)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyWasDown)(u32 sub_id, u32 key_code);
typedef bool (*InputKeyWasUp)(u32 sub_id, u32 key_code);
typedef f32 (*InputKeyGetCurrentValue)(u32 sub_id, u32 key_code);
typedef f32 (*InputKeyGetPreviousValue)(u32 sub_id, u32 key_code);
typedef void (*InputProcessKey)(u32 sub_id, u32 key_code, bool is_down);
typedef void* (*InputGetCurrentState)(u32 sub_id);
typedef void* (*InputGetPreviousState)(u32 sub_id);

typedef struct InputDevice 
{
    u32 id;
    u32 sub_id;
    
    InputKeyDown key_down;
    InputKeyUp key_up;
    InputKeyWasDown key_was_down;
    InputKeyWasUp key_was_up;
    
    InputKeyGetCurrentValue get_current_key_value;
    InputKeyGetPreviousValue get_previous_key_value;

    InputProcessKey process_key;

    InputGetCurrentState get_current_state;
    InputGetPreviousState get_previous_state;
    
    u64 state_size;
} InputDevice;

#define DEVICE_VALID(device) (device.id != 0 && device.key_down && device.key_up && device.key_was_down && device.key_was_up && device.process_key)
#define IS_SAME_DEVICE(device, dev_id, sub_id) (device.id == (dev_id) && device.sub_id == (sub_id))
#define DEVICE_SUB_ID_ANY -1

typedef struct InputMapping 
{
    u32 device_id;
    i32 sub_id;
    u32 key_code;
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