#include "core/input/input.h"
#include "input_mouse.h"
#include "core/event.h"
#include "core/memory.h"

typedef struct MouseState
{
    i32 x;
    i32 y;
    bool buttons[MOUSE_BUTTON_COUNT];
} MouseState;

static MouseState current_mouse_state;
static MouseState previous_mouse_state;

void mouse_register(u32 sub_id)
{
    InputDevice device = {0};
    device.id = MOUSE_DEVICE_ID;
    device.sub_id = sub_id;

    device.key_down = mouse_button_down;
    device.key_up = mouse_button_up;
    device.key_was_down = mouse_button_was_down;
    device.key_was_up = mouse_button_was_up;
    device.process_key = mouse_process_button;
    device.get_current_state = mouse_get_current_state;
    device.get_previous_state = mouse_get_previous_state;
    device.get_current_key_value = mouse_button_current_value;
    device.get_previous_key_value = mouse_button_previous_value;
    device.is_connected = mouse_is_connected;
    device.state_size = sizeof(MouseState); 

    input_register_device(device);
}

void* mouse_get_current_state(u32 sub_id)
{
    (void) sub_id;
    return (void*) &current_mouse_state;
}

void* mouse_get_previous_state(u32 sub_id)
{
    (void) sub_id;
    return (void*) &previous_mouse_state;
}

void mouse_process_button(u32 sub_id, u32 button, bool is_down)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    if (state->buttons[button] == is_down)
    {
        return;
    }

    state->buttons[button] = is_down;

    EventContext context = {0};
    context.data.u16[0] = button;

    event_trigger(is_down ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
}

void mouse_process_mouse_move(u32 sub_id, i32 x, i32 y)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
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

void mouse_process_mouse_wheel(u32 sub_id, i8 z_delta)
{
    EventContext context = {0};
    context.data.i8[0] = z_delta;

    event_trigger(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

f32 mouse_button_current_value(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    return state->buttons[button] ? 1.0f : 0.0f;
}

f32 mouse_button_previous_value(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    return state->buttons[button] ? 1.0f : 0.0f;
}

bool mouse_button_down(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    return state->buttons[button];
}

bool mouse_button_up(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    return !state->buttons[button];
}

bool mouse_button_was_down(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID, sub_id);
    return state->buttons[button];
}

bool mouse_button_was_up(u32 sub_id, u32 button)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID, sub_id);
    return !state->buttons[button];
}

void input_get_mouse_position(u32 sub_id, i32* x, i32* y)
{
    MouseState* state = (MouseState*) input_get_current_state(MOUSE_DEVICE_ID, sub_id);
    *x = state->x;
    *y = state->y;
}

void input_get_previous_mouse_position(u32 sub_id, i32* x, i32* y)
{
    MouseState* state = (MouseState*) input_get_previous_state(MOUSE_DEVICE_ID, sub_id);
    *x = state->x;
    *y = state->y;
}

bool mouse_is_connected(u32 sub_id)
{
    (void) sub_id;
    return true;
}