#pragma once

#include "defines.h"
#include "input_defines.h"
#include "devices/input_devices.h"

typedef struct InputSystemConfig
{
    u8 max_devices;
    u8 max_binded_actions;
} InputSystemConfig;

void input_init(void* state, InputSystemConfig config);
void input_shutdown(void);
void input_update(f64 delta_time);

u64 input_get_state_size(InputSystemConfig config);

void input_register_device(InputDevice device);
void input_unregister_device(u32 device_id);

KENZINE_API bool input_action_bind_button(const char* action_name, InputMapping mapping);

KENZINE_API bool input_action_bind_native_axis(const char* action_name, InputMapping mapping);
KENZINE_API bool input_action_bind_virtual_axis(const char* action_name, InputMapping positive_mapping, InputMapping negative_mapping);

KENZINE_API bool input_action_unbind_all_mappings(const char* action_name);
KENZINE_API void input_action_unbind_all_actions(void);
KENZINE_API bool input_action_get(const char* action_name, InputAction* out_action);
KENZINE_API bool input_action_get_bindings(const char* action_name, InputActionBinding** out_bindings, u32* out_count);

KENZINE_API bool input_action_down(const char* action_name, u32 sub_id);
KENZINE_API bool input_action_up(const char* action_name, u32 sub_id);
KENZINE_API bool input_action_was_down(const char* action_name, u32 sub_id);
KENZINE_API bool input_action_was_up(const char* action_name, u32 sub_id);

KENZINE_API bool input_action_started(const char* action_name, u32 sub_id);
KENZINE_API bool input_action_ended(const char* action_name, u32 sub_id);

KENZINE_API bool input_action_value(const char* action_name, u32 sub_id, f32* out_value);
KENZINE_API bool input_action_previous_value(const char* action_name, u32 sub_id, f32* out_value);
KENZINE_API bool input_action_delta(const char* action_name, u32 sub_id, f32* out_delta);

void* input_get_current_state(u32 device_id, u32 sub_id);
void* input_get_previous_state(u32 device_id, u32 sub_id);
void input_process_key(u32 device_id, u32 sub_id, u32 key_code, bool is_down);