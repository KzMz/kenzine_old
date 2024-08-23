#include "texture_system.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/containers/dyn_array.h"
#include "lib/containers/hash_table.h"
#include "lib/string.h"

#include "renderer/renderer_frontend.h"

#include "systems/resource_system.h"

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
void destroy_texture(Texture* texture);

bool texture_system_init(void* state, TextureSystemConfig config)
{
    if (config.max_textures == 0)
    {
        log_error("Texture system config is invalid. Max textures must be greater than 0.");
        return false;
    }

    texture_system_state = (TextureSystemState*) state;
    texture_system_state->config = config;
    texture_system_state->textures = state + sizeof(TextureSystemState);

    // See if this needs to be near the other memory
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

    memory_zero(texture_system_state->textures, sizeof(Texture) * texture_system_state->config.max_textures);

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

    char name_copy[TEXTURE_NAME_MAX_LENGTH];
    string_copy_n(name_copy, name, TEXTURE_NAME_MAX_LENGTH);

    ref.reference_count--;
    if (ref.reference_count == 0 && ref.auto_release)
    {
        Texture* t = &texture_system_state->textures[ref.handle];
        destroy_texture(t);

        ref.handle = INVALID_ID;
        ref.auto_release = false;
    }

    hashtable_set(&texture_system_state->texture_table, name_copy, &ref);
}

u64 texture_system_get_state_size(TextureSystemConfig config)
{
    return sizeof(TextureSystemState) + sizeof(Texture) * config.max_textures;
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
    u8 pixels[DEFAULT_TEXTURE_PIXELS_COUNT];
    for (u32 i = 0; i < DEFAULT_TEXTURE_PIXELS_COUNT; i++)
    {
        pixels[i] = 255;
    }

    for (u64 row = 0; row < DEFAULT_TEXTURE_SIZE; row++)
    {
        for (u64 col = 0; col < DEFAULT_TEXTURE_SIZE; col++)
        {
            u64 index = (row * DEFAULT_TEXTURE_SIZE + col) * DEFAULT_TEXTURE_BPP;
            if (row % 2 == col % 2)
            {
                pixels[index + 0] = 0;
                pixels[index + 1] = 0;
            }
        }
    }

    string_copy_n(state->default_texture.name, DEFAULT_TEXTURE_NAME, TEXTURE_NAME_MAX_LENGTH);
    state->default_texture.width = DEFAULT_TEXTURE_SIZE;
    state->default_texture.height = DEFAULT_TEXTURE_SIZE;
    state->default_texture.channel_count = DEFAULT_TEXTURE_BPP;
    state->default_texture.generation = INVALID_ID;
    state->default_texture.has_transparency = false;

    renderer_create_texture(pixels, &state->default_texture);

    state->default_texture.generation = INVALID_ID;
    return true;
}

void destroy_default_texture(TextureSystemState* state)
{
    if (state == NULL) return;
    destroy_texture(&state->default_texture);
}

bool load_texture(const char* texture_name, Texture* out_texture)
{
    Resource image_resource;
    if (!resource_system_load(texture_name, RESOURCE_TYPE_IMAGE, &image_resource))
    {
        log_error("Failed to load texture: %s", texture_name);
        return false;
    }

    ImageResourceData* image_data = (ImageResourceData*) image_resource.data;

    Texture tmp;
    tmp.width = image_data->width;
    tmp.height = image_data->height;
    tmp.channel_count = image_data->channel_count;

    u32 generation = out_texture->generation;
    out_texture->generation = INVALID_ID;

    u64 total_size = tmp.width * tmp.height * tmp.channel_count;
    bool has_transparency = false;
    for (i64 i = 0; i < total_size; i += tmp.channel_count)
    {
        u8 alpha = image_data->pixels[i + 3];
        if (alpha < 255)
        {
            has_transparency = true;
            break;
        }
    }

    string_copy_n(tmp.name, texture_name, TEXTURE_NAME_MAX_LENGTH);
    tmp.generation = INVALID_ID;
    tmp.has_transparency = has_transparency;

    renderer_create_texture(image_data->pixels, &tmp);

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

    resource_system_unload(&image_resource);
    return true;
}

void destroy_texture(Texture* texture)
{
    if (texture == NULL) 
    {
        return;
    }
    if (texture->id == INVALID_ID)
    {
        return;
    }

    renderer_destroy_texture(texture);
    memory_zero(texture->name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);
    memory_zero(texture, sizeof(Texture));
    texture->id = INVALID_ID;
    texture->generation = INVALID_ID;
}