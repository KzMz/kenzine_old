#include "input.h"
#include "input_mouse.h"
#include "core/event.h"
#include "core/memory.h"

void mouse_register(void)
{
    InputDevice device = {0};
    device.id = MOUSE_DEVICE_ID;

    device.key_down = mouse_button_down;
    device.key_up = mouse_button_up;
    device.key_was_down = mouse_button_was_down;
    device.key_was_up = mouse_button_was_up;
    device.process_key = mouse_process_button;

    device.current_state = memory_alloc(sizeof(MouseState), MEMORY_TAG_INPUTDEVICE);
    memory_zero(device.current_state, sizeof(MouseState));

    device.previous_state = memory_alloc(sizeof(MouseState), MEMORY_TAG_INPUTDEVICE);
    memory_zero(device.previous_state, sizeof(MouseState));

    input_register_device(device);
}

void mouse_process_button(u32 button, bool is_down)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID);
    if (state->buttons[button] == is_down)
    {
        return;
    }

    state->buttons[button] = is_down;

    EventContext context = {0};
    context.data.u16[0] = button;

    event_trigger(is_down ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
}

void mouse_process_mouse_move(i32 x, i32 y)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID);
    if (!state) return;

    if (state->x == x && state->y == y)
    {
        return;
    }

    state->x = x;
    state->y = y;

    EventContext context = {0};
    context.data.i32[0] = x;
    context.data.i32[1] = y;

    event_trigger(EVENT_CODE_MOUSE_MOVED, 0, context);
}

void mouse_process_mouse_wheel(i8 z_delta)
{
    EventContext context = {0};
    context.data.i8[0] = z_delta;

    event_trigger(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

bool mouse_button_down(u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID);
    return state->buttons[button];
}

bool mouse_button_up(u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID);
    return !state->buttons[button];
}

bool mouse_button_was_down(u32 button)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID);
    return state->buttons[button];
}

bool mouse_button_was_up(u32 button)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID);
    return !state->buttons[button];
}

void input_get_mouse_position(i32* x, i32* y)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID);
    *x = state->x;
    *y = state->y;
}

void input_get_previous_mouse_position(i32* x, i32* y)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID);
    *x = state->x;
    *y = state->y;
}