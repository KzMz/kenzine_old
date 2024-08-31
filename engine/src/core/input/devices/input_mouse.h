#pragma once

#include "defines.h"

#define MOUSE_DEVICE_ID 1

typedef enum MouseButton
{
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_COUNT
} MouseButton;

bool mouse_button_down(u32 sub_id, u32 button);
bool mouse_button_up(u32 sub_id, u32 button);
bool mouse_button_was_down(u32 sub_id, u32 button);
bool mouse_button_was_up(u32 sub_id, u32 button);

f32 mouse_button_current_value(u32 sub_id, u32 button);
f32 mouse_button_previous_value(u32 sub_id, u32 button);

void mouse_process_button(u32 sub_id, u32 button, bool is_down);

KENZINE_API void input_get_mouse_position(u32 sub_id, i32* x, i32* y);
KENZINE_API void input_get_previous_mouse_position(u32 sub_id, i32* x, i32* y);
void mouse_process_mouse_move(u32 sub_id, i32 x, i32 y);
void mouse_process_mouse_wheel(u32 sub_id, i8 z_delta);

void mouse_register(u32 sub_id);
void* mouse_get_current_state(u32 sub_id);
void* mouse_get_previous_state(u32 sub_id);

bool mouse_is_connected(u32 sub_id);