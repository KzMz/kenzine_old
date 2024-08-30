#include "core/input/input.h"
#include "input_keyboard.h"
#include "core/event.h"
#include "core/memory.h"
#include <stdio.h>
#include <string.h>

static KeyboardState current_keyboard_state;
static KeyboardState previous_keyboard_state;

void keyboard_register(void)
{
    InputDevice device = {0};
    device.id = KEYBOARD_DEVICE_ID;
    
    device.key_down = keyboard_key_down;
    device.key_up = keyboard_key_up;
    device.key_was_down = keyboard_key_was_down;
    device.key_was_up = keyboard_key_was_up;
    device.process_key = keyboard_process_key;
    device.get_current_state = keyboard_get_current_state;
    device.get_previous_state = keyboard_get_previous_state;
    device.get_current_key_value = keyboard_key_current_value;
    device.get_previous_key_value = keyboard_key_previous_value;
    device.state_size = sizeof(KeyboardState);

    input_register_device(device);
}

void* keyboard_get_current_state(void)
{
    return (void*) &current_keyboard_state;
}

void* keyboard_get_previous_state(void)
{
    return (void*) &previous_keyboard_state;
}

void keyboard_process_key(u32 key, bool is_down)
{
    if (key == KEY_LALT)
    {
        
    }
    else if (key == KEY_RALT)
    {
        
    }
    else if (key == KEY_LSHIFT)
    {
        
    }
    else if (key == KEY_RSHIFT)
    {
        
    }
    else if (key == KEY_LCONTROL)
    {
        
    }
    else if (key == KEY_RCONTROL)
    {
        
    }

    KeyboardState* state = (KeyboardState*) input_get_current_state(KEYBOARD_DEVICE_ID);
    if (state->keys[key] == is_down) 
    {
        return;
    }

    state->keys[key] = is_down;

    EventContext context = {0};
    context.data.u16[0] = key;

    event_trigger(is_down ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
}

bool keyboard_key_down(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_current_state(KEYBOARD_DEVICE_ID);
    return state->keys[key];
}

bool keyboard_key_up(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_current_state(KEYBOARD_DEVICE_ID);
    return !state->keys[key];
}

bool keyboard_key_was_down(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_previous_state(KEYBOARD_DEVICE_ID);
    return state->keys[key];
}

bool keyboard_key_was_up(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_previous_state(KEYBOARD_DEVICE_ID);
    return !state->keys[key];
}

f32 keyboard_key_current_value(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_current_state(KEYBOARD_DEVICE_ID);
    return state->keys[key] ? 1.0f : 0.0f;
}

f32 keyboard_key_previous_value(u32 key)
{
    KeyboardState* state = (KeyboardState*) input_get_previous_state(KEYBOARD_DEVICE_ID);
    return state->keys[key] ? 1.0f : 0.0f;
}