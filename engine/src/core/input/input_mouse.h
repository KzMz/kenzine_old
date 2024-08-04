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

typedef struct MouseState
{
    i32 x;
    i32 y;
    bool buttons[MOUSE_BUTTON_COUNT];
} MouseState;

bool mouse_button_down(u32 button);
bool mouse_button_up(u32 button);
bool mouse_button_was_down(u32 button);
bool mouse_button_was_up(u32 button);

void mouse_process_button(u32 button, bool is_down);

KENZINE_API void input_get_mouse_position(i32* x, i32* y);
KENZINE_API void input_get_previous_mouse_position(i32* x, i32* y);
void mouse_process_mouse_move(i32 x, i32 y);
void mouse_process_mouse_wheel(i8 z_delta);

void mouse_register(void);