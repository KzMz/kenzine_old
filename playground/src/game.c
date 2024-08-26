#include "game.h"
#include <core/log.h>
#include <core/input/input.h>
#include <core/memory.h>

#include <renderer/renderer_frontend.h>
#include <lib/math/mat4.h>
#include <core/event.h>

void update_view_matrix(Game* game)
{
    GameState* state = (GameState*) game->state;
    if (state->camera_view_dirty == false)
    {
        return;
    }
    
    Mat4 rotation = mat4_euler_rotation(state->camera_euler.x, state->camera_euler.y, state->camera_euler.z);
    Mat4 translation = mat4_translation(state->camera_position);

    Mat4 view = mat4_mul(rotation, translation);
    state->view = mat4_inverse(view);

    state->camera_view_dirty = false;
}

void camera_yaw(Game* game, f32 angle)
{
    GameState* state = (GameState*) game->state;
    state->camera_euler.y += angle;
    state->camera_view_dirty = true;
}

void camera_pitch(Game* game, f32 angle)
{
    GameState* state = (GameState*) game->state;
    state->camera_euler.x += angle;
    f32 max_angle = deg_to_rad(89.0f);
    state->camera_euler.x = kz_clamp(state->camera_euler.x, -max_angle, max_angle);

    state->camera_view_dirty = true;
}

bool game_init(Game* game)
{
    log_info("Game initialized");

    GameState* state = (GameState*) game->state;
    state->camera_position = (Vec3) { 0, 0, 30 };
    state->camera_euler = (Vec3) { 0, 0, 0 };
    state->camera_view_dirty = true;

    update_view_matrix(game);

    return true;
}

bool game_update(Game* game, f64 delta_time)
{
    if (input_key_was_down(KEYBOARD_DEVICE_ID, KEY_M) && input_key_up(KEYBOARD_DEVICE_ID, KEY_M))
    {
        log_debug(get_memory_report());
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_A) || input_key_down(KEYBOARD_DEVICE_ID, KEY_LEFT))
    {
        camera_yaw(game, 1.0f * delta_time);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_D) || input_key_down(KEYBOARD_DEVICE_ID, KEY_RIGHT))
    {
        camera_yaw(game, -1.0f * delta_time);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_UP))
    {
        camera_pitch(game, 1.0f * delta_time);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_DOWN))
    {
        camera_pitch(game, -1.0f * delta_time);
    }

    if (input_key_up(KEYBOARD_DEVICE_ID, KEY_T) && input_key_was_down(KEYBOARD_DEVICE_ID, KEY_T))
    {
        EventContext context = { 0 };
        event_trigger(EVENT_CODE_DEBUG0, game, context);
    }

    f32 temp_speed = 50.0f;
    Vec3 velocity = vec3_zero();

    GameState* state = (GameState*) game->state;
    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_W))
    {
        Vec3 forward = mat4_forward(state->view);
        velocity = vec3_add(velocity, forward);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_S))
    {
        Vec3 backward = mat4_backward(state->view);
        velocity = vec3_add(velocity, backward);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_Q))
    {
        Vec3 left = mat4_left(state->view);
        velocity = vec3_add(velocity, left);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_E))
    {
        Vec3 right = mat4_right(state->view);
        velocity = vec3_add(velocity, right);
    }

    if (input_key_down(KEYBOARD_DEVICE_ID, KEY_SPACE))
    {
        velocity.y += 1.0f;
    }

    Vec3 z = vec3_zero();
    if (!vec3_equals(z, velocity, 0.0002f))
    {
        vec3_normalize(&velocity);
        state->camera_position.x += velocity.x * temp_speed * delta_time;
        state->camera_position.y += velocity.y * temp_speed * delta_time;
        state->camera_position.z += velocity.z * temp_speed * delta_time;
        state->camera_view_dirty = true;
    }

    update_view_matrix(game);

    renderer_set_view(state->view, state->camera_position);

    return true;
}

bool game_render(Game* game, f64 delta_time)
{
    //log_info("Game rendered");
    return true;
}

void game_resize(Game* game, i32 width, i32 height)
{
    log_info("Game resized");
}

void game_shutdown(Game* game)
{

}