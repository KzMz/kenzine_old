#include "input.h"
#include "input_keyboard.h"
#include "core/event.h"
#include "core/memory.h"

void keyboard_register(void)
{
    InputDevice device = {0};
    device.id = KEYBOARD_DEVICE_ID;
    
    device.key_down = keyboard_key_down;
    device.key_up = keyboard_key_up;
    device.key_was_down = keyboard_key_was_down;
    device.key_was_up = keyboard_key_was_up;
    device.process_key = keyboard_process_key;

    device.current_state = memory_alloc(sizeof(KeyboardState), MEMORY_TAG_INPUTDEVICE);
    memory_zero(device.current_state, sizeof(KeyboardState));

    device.previous_state = memory_alloc(sizeof(KeyboardState), MEMORY_TAG_INPUTDEVICE);
    memory_zero(device.previous_state, sizeof(KeyboardState));

    input_register_device(device);
}

void keyboard_process_key(u32 key, bool is_down)
{
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