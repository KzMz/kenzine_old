#include "input.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/event.h"
#include "lib/containers/hash_table.h"
#include "lib/string.h"

#include <stddef.h>

typedef struct InputState 
{
    HashTable input_actions;
    InputDevice* input_devices;
    u8 max_devices;
} InputState;

static InputState* input_state = 0;

bool input_key_down(u32 device_id, u32 sub_id, u32 key_code);
bool input_key_up(u32 device_id, u32 sub_id, u32 key_code);
bool input_key_was_down(u32 device_id, u32 sub_id, u32 key_code);
bool input_key_was_up(u32 device_id, u32 sub_id, u32 key_code);

f32 input_key_value(u32 device_id, u32 sub_id, u32 key_code);
f32 input_key_previous_value(u32 device_id, u32 sub_id, u32 key_code);

bool input_action_bind_button(const char* action_name, InputMapping mapping)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);

    if (action.type != INPUT_ACTION_TYPE_NONE)
    {
        if (action.type != INPUT_ACTION_TYPE_BUTTON)
        {
            log_error("Action %s is already bound to a different type", action_name);
            return false;
        }
        if (action.bindings_count >= MAX_INPUTACTION_BINDINGS)
        {
            log_error("Action %s has reached the maximum number of bindings", action_name);
            return false;
        }

        action.bindings[action.bindings_count++].mapping0 = mapping;
    } 
    else 
    {
        action.type = INPUT_ACTION_TYPE_BUTTON;
        string_copy_n(action.name, action_name, MAX_INPUTACTION_NAME_LENGTH);
        action.bindings[0].mapping0 = mapping;
        action.bindings_count = 1;
    }

    hashtable_set(&input_state->input_actions, action_name, &action);
    return true;
}

KENZINE_API bool input_action_bind_native_axis(const char* action_name, InputMapping mapping)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);

    if (action.type != INPUT_ACTION_TYPE_NONE)
    {
        if (action.type != INPUT_ACTION_TYPE_AXIS)
        {
            log_error("Action %s is already bound to a different type", action_name);
            return false;
        }
        if (action.bindings_count >= MAX_INPUTACTION_BINDINGS)
        {
            log_error("Action %s has reached the maximum number of bindings", action_name);
            return false;
        }

        action.bindings[action.bindings_count].mapping0 = mapping;
        action.bindings[action.bindings_count].axis_type = INPUT_ACTION_AXIS_TYPE_NATIVE;
        action.bindings_count++;
    } 
    else 
    {
        action.type = INPUT_ACTION_TYPE_AXIS;
        string_copy_n(action.name, action_name, MAX_INPUTACTION_NAME_LENGTH);
        action.bindings[0].mapping0 = mapping;
        action.bindings[0].axis_type = INPUT_ACTION_AXIS_TYPE_NATIVE;
        action.bindings_count = 1;
    }

    hashtable_set(&input_state->input_actions, action_name, &action);
    return true;
}

KENZINE_API bool input_action_bind_virtual_axis(const char* action_name, InputMapping positive_mapping, InputMapping negative_mapping)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);

    if (action.type != INPUT_ACTION_TYPE_NONE)
    {
        if (action.type != INPUT_ACTION_TYPE_AXIS)
        {
            log_error("Action %s is already bound to a different type", action_name);
            return false;
        }
        if (action.bindings_count >= MAX_INPUTACTION_BINDINGS)
        {
            log_error("Action %s has reached the maximum number of bindings", action_name);
            return false;
        }

        action.bindings[action.bindings_count].mapping0 = positive_mapping;
        action.bindings[action.bindings_count].mapping1 = negative_mapping;
        action.bindings[action.bindings_count].axis_type = INPUT_ACTION_AXIS_TYPE_VIRTUAL;
        action.bindings_count++;
    } 
    else 
    {
        action.type = INPUT_ACTION_TYPE_AXIS;
        string_copy_n(action.name, action_name, MAX_INPUTACTION_NAME_LENGTH);
        action.bindings[0].mapping0 = positive_mapping;
        action.bindings[0].mapping1 = negative_mapping;
        action.bindings[0].axis_type = INPUT_ACTION_AXIS_TYPE_VIRTUAL;
        action.bindings_count = 1;
    }

    hashtable_set(&input_state->input_actions, action_name, &action);
    return true;
}

bool input_action_unbind_all_mappings(const char* action_name)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);

    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    action.type = INPUT_ACTION_TYPE_NONE;
    action.bindings_count = 0;
    hashtable_set(&input_state->input_actions, action_name, &action);
    return true;
}

bool input_action_get(const char* action_name, InputAction* out_action)
{
    hashtable_get(&input_state->input_actions, action_name, out_action);
    return out_action->type != INPUT_ACTION_TYPE_NONE;
}

bool input_action_get_bindings(const char* action_name, InputActionBinding** out_bindings, u32* out_count)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);

    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    *out_bindings = action.bindings;
    *out_count = action.bindings_count;
    return true;
}

bool input_action_value(const char* action_name, u32 sub_id, f32* out_value)
{
    if (!out_value)
    {
        log_error("Invalid output value");
        return false;
    }

    *out_value = 0.0f;

    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_AXIS)
    {
        f32 value = 0.0f;
        u8 count = 0;
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (binding->axis_type == INPUT_ACTION_AXIS_TYPE_NATIVE)
            {
                f32 tmp = input_key_previous_value(binding->mapping0.device_id, sub_id, binding->mapping0.key_code);
                if (tmp != 0.0f)
                {
                    value += tmp;
                    count++;
                }
            }
            else 
            {
                f32 positive = input_key_previous_value(binding->mapping0.device_id, sub_id, binding->mapping0.key_code);
                f32 negative = input_key_previous_value(binding->mapping1.device_id, sub_id, binding->mapping1.key_code);
                value += positive - negative;
                count++;
            }
        }
        *out_value = count > 0 ? value / count : 0.0f;
        return true;
    }
    else if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_down(action.bindings[i].mapping0.device_id, sub_id, action.bindings[i].mapping0.key_code))
            {
                *out_value = 1.0f;
                return true;
            }
        }
    }

    return false;
}

bool input_action_previous_value(const char* action_name, u32 sub_id, f32* out_value)
{
    if (!out_value)
    {
        log_error("Invalid output value");
        return false;
    }

    *out_value = 0.0f;

    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_AXIS)
    {
        f32 value = 0.0f;
        u8 count = 0;
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (binding->axis_type == INPUT_ACTION_AXIS_TYPE_NATIVE)
            {
                f32 tmp = input_key_previous_value(binding->mapping0.device_id, sub_id, binding->mapping0.key_code);
                if (tmp != 0.0f)
                {
                    value += tmp;
                    count++;
                }
            }
            else 
            {
                f32 positive = input_key_previous_value(binding->mapping0.device_id, sub_id, binding->mapping0.key_code);
                f32 negative = input_key_previous_value(binding->mapping1.device_id, sub_id, binding->mapping1.key_code);
                value += positive - negative;
                count++;
            }
        }
        *out_value = count > 0 ? value / count : 0.0f;
        return true;
    }
    else if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_was_down(binding->mapping0.device_id, sub_id, binding->mapping0.key_code))
            {
                *out_value = 1.0f;
                return true;
            }
        }
    }

    return false;
}

bool input_action_delta(const char* action_name, u32 sub_id, f32* out_delta)
{
    if (!out_delta)
    {
        log_error("Invalid output value");
        return false;
    }

    *out_delta = 0.0f;

    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    f32 prev = 0.0f, current = 0.0f;
    input_action_value(action_name, sub_id, &current);
    input_action_previous_value(action_name, sub_id, &prev);

    *out_delta = current - prev;
    return true;
}

bool input_action_down(const char* action_name, u32 sub_id)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_down(binding->mapping0.device_id, sub_id, binding->mapping0.key_code))
            {
                return true;
            }
        }
    }

    return false;
}

bool input_action_up(const char* action_name, u32 sub_id)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_up(binding->mapping0.device_id, sub_id, binding->mapping0.key_code))
            {
                return true;
            }
        }
    }

    return false;
}

bool input_action_was_down(const char* action_name, u32 sub_id)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_was_down(binding->mapping0.device_id, sub_id, binding->mapping0.key_code))
            {
                return true;
            }
        }
    }

    return false;
}

bool input_action_was_up(const char* action_name, u32 sub_id)
{
    InputAction action = {0};
    hashtable_get(&input_state->input_actions, action_name, &action);
    if (action.type == INPUT_ACTION_TYPE_NONE)
    {
        log_error("Action %s not found", action_name);
        return false;
    }

    if (action.type == INPUT_ACTION_TYPE_BUTTON)
    {
        for (u32 i = 0; i < action.bindings_count; ++i)
        {
            InputActionBinding* binding = &action.bindings[i];
            if (binding->mapping0.sub_id != DEVICE_SUB_ID_ANY && 
                binding->mapping0.sub_id != sub_id)
            {
                continue;
            }

            if (input_key_was_up(binding->mapping0.device_id, sub_id, binding->mapping0.key_code))
            {
                return true;
            }
        }
    }

    return false;
}

void input_action_unbind_all_actions(void)
{
    hashtable_destroy(&input_state->input_actions);
}

bool input_action_started(const char* action_name, u32 sub_id)
{
    return input_action_down(action_name, sub_id) && input_action_was_up(action_name, sub_id);
}

bool input_action_ended(const char* action_name, u32 sub_id)
{
    return input_action_up(action_name, sub_id) && input_action_was_down(action_name, sub_id);
}

void input_register_device(InputDevice device)
{
    if (!DEVICE_VALID(device))
    {
        log_error("Invalid input device %u", device.id);
        return;
    }

    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (input_state->input_devices[i].id == 0)
        {
            input_state->input_devices[i] = device;
            break;
        }
    }
}

void input_unregister_device(u32 device_id)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (input_state->input_devices[i].id == device_id)
        {
            input_state->input_devices[i] = (InputDevice) {0};
            break;
        }
    }
}

bool input_key_down(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].key_down(sub_id, key_code);
        }
    }

    return false;
}

bool input_key_up(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].key_up(sub_id, key_code);
        }
    }

    return false;
}

bool input_key_was_down(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].key_was_down(sub_id, key_code);
        }
    }

    return false;
}

bool input_key_was_up(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].key_was_up(sub_id, key_code);
        }
    }

    return false;
}

f32 input_key_value(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].get_current_key_value(sub_id, key_code);
        }
    }

    return 0.0f;
}

f32 input_key_previous_value(u32 device_id, u32 sub_id, u32 key_code)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].get_previous_key_value(sub_id, key_code);
        }
    }

    return 0.0f;
}

void input_process_key(u32 device_id, u32 sub_id, u32 key_code, bool is_down)
{
    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (input_state->input_devices[i].id == device_id)
        {
            input_state->input_devices[i].process_key(sub_id, key_code, is_down);
            break;
        }
    }
}

void input_init(void* state, InputSystemConfig config) 
{
    input_state = (InputState*) state;
    input_state->max_devices = config.max_devices;
    hashtable_create(InputAction, config.max_binded_actions, false, &input_state->input_actions);

    InputAction empty = {0};
    hashtable_fill_with_value(&input_state->input_actions, &empty);
    
    input_state->input_devices = (InputDevice*) (state + sizeof(InputState));
    memory_zero(input_state->input_devices, sizeof(InputDevice) * config.max_devices);

    keyboard_register(0);
    mouse_register(0);
}

void input_shutdown(void)
{
    if (!input_state)
    {
        log_error("Input system not initialized.");
        return;
    }

    hashtable_destroy(&input_state->input_actions);

    if (input_state->input_devices != NULL)
    {
        memory_zero(input_state->input_devices, sizeof(InputDevice) * input_state->max_devices);
        input_state->input_devices = NULL;
    }

    input_state = NULL;
}

void input_update(f64 delta_time)
{
    if (!input_state)
    {
        return;
    }

    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (input_state->input_devices[i].id != 0)
        {
            void* current_state = input_state->input_devices[i].get_current_state(input_state->input_devices[i].sub_id);
            void* previous_state = input_state->input_devices[i].get_previous_state(input_state->input_devices[i].sub_id);
            if (!current_state || !previous_state)
            {
                continue;
            } 

            memory_copy(previous_state, current_state, input_state->input_devices[i].state_size);
        }
    }
}

void* input_get_current_state(u32 device_id, u32 sub_id)
{
    if (!input_state)
    {
        return 0;
    }

    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].get_current_state(sub_id);
        }
    }

    return 0;
}

void* input_get_previous_state(u32 device_id, u32 sub_id)
{
    if (!input_state)
    {
        return 0;
    }

    for (u32 i = 0; i < input_state->max_devices; ++i)
    {
        if (IS_SAME_DEVICE(input_state->input_devices[i], device_id, sub_id))
        {
            return input_state->input_devices[i].get_previous_state(sub_id);
        }
    }

    return 0;
}

u64 input_get_state_size(InputSystemConfig config)
{
    return sizeof(InputState) + 
           sizeof(InputDevice) * config.max_devices;
}