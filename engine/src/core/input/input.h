#pragma once

#include "defines.h"
#include "input_mouse.h"
#include "input_keyboard.h"

void input_init(void);
void input_shutdown(void);
void input_update(f64 delta_time);

typedef bool (*InputKeyDown)(u32 key_code);
typedef bool (*InputKeyUp)(u32 key_code);
typedef bool (*InputKeyWasDown)(u32 key_code);
typedef bool (*InputKeyWasUp)(u32 key_code);
typedef void (*InputProcessKey)(u32 key_code, bool is_down);

typedef struct InputDevice {
    u32 id;
    
    InputKeyDown key_down;
    InputKeyUp key_up;
    InputKeyWasDown key_was_down;
    InputKeyWasUp key_was_up;
    InputProcessKey process_key;

    void* current_state;
    void* previous_state;
} InputDevice;

#define DEVICE_VALID(device) (device.id != 0 && device.key_down && device.key_up && device.key_was_down && device.key_was_up && device.process_key)

KENZINE_API bool input_key_down(u32 device_id, u32 key_code);
KENZINE_API bool input_key_up(u32 device_id, u32 key_code);
KENZINE_API bool input_key_was_down(u32 device_id, u32 key_code);
KENZINE_API bool input_key_was_up(u32 device_id, u32 key_code);
void input_process_key(u32 device_id, u32 key_code, bool is_down);

KENZINE_API void input_register_device(InputDevice device);
KENZINE_API void input_unregister_device(u32 device_id);

KENZINE_API void* input_get_current_state(u32 device_id);
KENZINE_API void* input_get_previous_state(u32 device_id);