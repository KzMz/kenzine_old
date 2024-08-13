#include "input.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/event.h"

#define MAX_INPUT_DEVICES 4

static InputDevice input_devices[MAX_INPUT_DEVICES] = {0};
static bool initialized = false;

void input_register_device(InputDevice device)
{
    if (!DEVICE_VALID(device))
    {
        log_error("Invalid input device %u", device.id);
        return;
    }

    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == 0)
        {
            input_devices[i] = device;
            break;
        }
    }
}

void input_unregister_device(u32 device_id)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            input_devices[i] = (InputDevice) {0};
            break;
        }
    }
}

bool input_key_down(u32 device_id, u32 key_code)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].key_down(key_code);
        }
    }

    return false;
}

bool input_key_up(u32 device_id, u32 key_code)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].key_up(key_code);
        }
    }

    return false;
}

bool input_key_was_down(u32 device_id, u32 key_code)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].key_was_down(key_code);
        }
    }

    return false;
}

bool input_key_was_up(u32 device_id, u32 key_code)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].key_was_up(key_code);
        }
    }

    return false;
}

void input_process_key(u32 device_id, u32 key_code, bool is_down)
{
    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            input_devices[i].process_key(key_code, is_down);
            break;
        }
    }
}

void input_init(void) 
{
    if (initialized)
    {
        log_error("Input system already initialized.");
        return;
    }

    memory_zero(input_devices, sizeof(input_devices));

    keyboard_register();
    mouse_register();

    initialized = true;
}

void input_shutdown(void)
{
    if (!initialized)
    {
        log_error("Input system not initialized.");
        return;
    }

    initialized = false;
}

void input_update(f64 delta_time)
{
    if (!initialized)
    {
        return;
    }

    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id != 0)
        {
            void* current_state = input_devices[i].get_current_state();
            void* previous_state = input_devices[i].get_previous_state();
            if (!current_state || !previous_state)
            {
                continue;
            } 

            memory_copy(previous_state, current_state, input_devices[i].state_size);
        }
    }
}

void* input_get_current_state(u32 device_id)
{
    if (!initialized)
    {
        return 0;
    }

    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].get_current_state();
        }
    }

    return 0;
}

void* input_get_previous_state(u32 device_id)
{
    if (!initialized)
    {
        return 0;
    }

    for (u32 i = 0; i < MAX_INPUT_DEVICES; ++i)
    {
        if (input_devices[i].id == device_id)
        {
            return input_devices[i].get_previous_state();
        }
    }

    return 0;
}