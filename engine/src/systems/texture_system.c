#include "texture_system.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/containers/dyn_array.h"
#include "lib/containers/hash_table.h"
#include "lib/string.h"

#include "renderer/renderer_frontend.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#include <stddef.h>

typedef struct TextureSystemState
{
    TextureSystemConfig config;
    Texture default_texture;

    Texture* textures;
    HashTable texture_table;
} TextureSystemState;

typedef struct TextureReference
{
    u64 reference_count;
    u32 handle;
    bool auto_release;
} TextureReference;

static TextureSystemState* texture_system_state = NULL;

void create_texture(Texture* t)
{
    memory_zero(t, sizeof(Texture));
    t->generation = INVALID_ID;
}

bool create_default_texture(TextureSystemState* state);
void destroy_default_texture(TextureSystemState* state);
bool load_texture(const char* texture_name, Texture* out_texture);

bool texture_system_init(void* state, TextureSystemConfig config)
{
    if (config.max_textures == 0)
    {
        log_error("Texture system config is invalid. Max textures must be greater than 0.");
        return false;
    }

    // NOTE: see if is it worth to have all this memory near one another
    texture_system_state = (TextureSystemState*) state;
    texture_system_state->config = config;
    texture_system_state->textures = dynarray_reserve(Texture, config.max_textures);
    hashtable_create(TextureReference, config.max_textures, false, &texture_system_state->texture_table);

    TextureReference invalid_ref;
    invalid_ref.reference_count = 0;
    invalid_ref.handle = INVALID_ID;
    invalid_ref.auto_release = false;
    hashtable_fill_with_value(&texture_system_state->texture_table, &invalid_ref);

    for (u32 i = 0; i < config.max_textures; ++i)
    {
        texture_system_state->textures[i].id = INVALID_ID;
        texture_system_state->textures[i].generation = INVALID_ID;
    }

    create_default_texture(texture_system_state);
    return true;
}

void texture_system_shutdown(void)
{
    if (texture_system_state == NULL) return;

    for (u32 i = 0; i < texture_system_state->config.max_textures; ++i)
    {
        Texture* texture = &texture_system_state->textures[i];
        if (texture->generation != INVALID_ID)
        {
            renderer_destroy_texture(texture);
        }
    }

    destroy_default_texture(texture_system_state);

    dynarray_destroy(texture_system_state->textures);
    hashtable_destroy(&texture_system_state->texture_table);
    memory_zero(texture_system_state, sizeof(TextureSystemState));
    texture_system_state = NULL;
}

Texture* texture_system_acquire(const char* name, bool auto_release)
{
    if (string_equals_nocase(name, DEFAULT_TEXTURE_NAME))
    {
        return &texture_system_state->default_texture;
    }

    TextureReference ref;
    hashtable_get(&texture_system_state->texture_table, name, &ref);

    if (ref.reference_count == 0)
    {
        ref.auto_release = auto_release;
    }
    ref.reference_count++;
    if (ref.handle == INVALID_ID)
    {
        u32 count = texture_system_state->config.max_textures;
        Texture* t;

        for (u32 i = 0; i < count; ++i)
        {
            if (texture_system_state->textures[i].id == INVALID_ID)
            {
                // Free slot
                ref.handle = i;
                t = &texture_system_state->textures[i];
                break;
            }
        }

        if (t == NULL || ref.handle == INVALID_ID)
        {
            log_fatal("Texture system is full. Cannot load texture: %s", name);
            return NULL;
        }

        if (!load_texture(name, t))
        {
            log_error("Failed to load texture: %s", name);
            return NULL;
        }

        t->id = ref.handle;
    }

    hashtable_set(&texture_system_state->texture_table, name, &ref);
    return &texture_system_state->textures[ref.handle];
}

void texture_system_release(const char* name)
{
    if (string_equals_nocase(name, DEFAULT_TEXTURE_NAME))
    {
        return;
    }

    TextureReference ref;
    hashtable_get(&texture_system_state->texture_table, name, &ref);

    if (ref.reference_count == 0)
    {
        log_warning("Texture: %s is not acquired.", name);
        return;
    }

    ref.reference_count--;
    if (ref.reference_count == 0 && ref.auto_release)
    {
        Texture* t = &texture_system_state->textures[ref.handle];
        renderer_destroy_texture(t);

        memory_zero(t, sizeof(Texture));
        t->id = INVALID_ID;
        t->generation = INVALID_ID;
        ref.handle = INVALID_ID;
        ref.auto_release = false;
    }

    hashtable_set(&texture_system_state->texture_table, name, &ref);
}

u64 texture_system_get_state_size(void)
{
    return sizeof(TextureSystemState);
}

Texture* texture_system_get_default(void)
{
    if (texture_system_state == NULL)
    {
        log_error("Texture system is not initialized.");
        return NULL;
    }

    return &texture_system_state->default_texture;
}

bool create_default_texture(TextureSystemState* state)
{
    // NOTE: Create default texture
    const u32 texture_size = 256;
    const u32 bpp = 4;
    const u32 pixels_count = texture_size * texture_size * bpp;
    u8 pixels[pixels_count];
    for (u32 i = 0; i < pixels_count; i++)
    {
        pixels[i] = 255;
    }

    for (u64 row = 0; row < texture_size; row++)
    {
        for (u64 col = 0; col < texture_size; col++)
        {
            u64 index = (row * texture_size + col) * bpp;
            if (row % 2 == col % 2)
            {
                pixels[index + 0] = 0;
                pixels[index + 1] = 0;
            }
        }
    }

    renderer_create_texture(
        DEFAULT_TEXTURE_NAME,
        texture_size, texture_size,
        bpp,
        pixels,
        false,
        &state->default_texture
    );

    state->default_texture.generation = INVALID_ID;
    return true;
}

void destroy_default_texture(TextureSystemState* state)
{
    if (state == NULL) return;
    renderer_destroy_texture(&state->default_texture);
}

bool load_texture(const char* texture_name, Texture* out_texture)
{
    char* format_str = "../assets/textures/%s.%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    
    const i32 max_path_length = 512;
    char path[max_path_length];
    string_format(path, format_str, texture_name, "png");

    Texture tmp;
    u8* data = stbi_load(
        path,
        (i32*) &tmp.width,
        (i32*) &tmp.height,
        (i32*) &tmp.channel_count,
        required_channel_count
    );

    tmp.channel_count = required_channel_count;

    if (data == NULL)
    {
        log_warning("Failed to load texture: %s. Reason: %s", path, stbi_failure_reason());
        return false;
    }

    u32 generation = out_texture->generation;
    out_texture->generation = INVALID_ID;

    u64 total_size = tmp.width * tmp.height * tmp.channel_count;
    bool has_transparency = false;
    for (i64 i = 0; i < total_size; i += tmp.channel_count)
    {
        u8 alpha = data[i + 3];
        if (alpha < 255)
        {
            has_transparency = true;
            break;
        }
    }

    if (stbi_failure_reason())
    {
        log_warning("Failed to load texture: %s. Reason: %s", path, stbi_failure_reason());
        return false;
    }

    renderer_create_texture(
        texture_name,
        tmp.width, tmp.height,  
        tmp.channel_count, 
        data,
        has_transparency,
        &tmp
    );

    Texture old = *out_texture;
    *out_texture = tmp;

    renderer_destroy_texture(&old);

    if (generation == INVALID_ID)
    {
        out_texture->generation = 0;
    }
    else
    {
        out_texture->generation = generation + 1;
    }

    stbi_image_free(data);
    return true;
}