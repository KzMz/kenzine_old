#pragma once
#include "lib/math/math_defines.h"

#define TEXTURE_NAME_MAX_LENGTH 512
#define MATERIAL_NAME_MAX_LENGTH 256
#define GEOMETRY_NAME_MAX_LENGTH 256

typedef enum ResourceType
{
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    RESOURCE_TYPE_CUSTOM
} ResourceType;

typedef struct Resource
{
    u64 loader_id;
    ResourceType type;
    const char* name;
    char* full_path;
    u64 size;
    void* data;
} Resource;

typedef struct ImageResourceData
{
    u8 channel_count;
    u32 width;
    u32 height;
    u8* pixels;
} ImageResourceData;

typedef struct Texture
{
    u32 id;
    u32 width;
    u32 height;
    u8 channel_count;
    bool has_transparency;
    u32 generation;
    char name[TEXTURE_NAME_MAX_LENGTH];
    void* data;
} Texture;

typedef enum TextureUsage
{
    TEXTURE_USE_UNKNOWN = 0x00,
    TEXTURE_USE_DIFFUSE,
} TextureUsage;

typedef struct TextureMap
{
    Texture* texture;
    TextureUsage usage;
} TextureMap;

typedef struct MaterialResourceData
{
    char name[MATERIAL_NAME_MAX_LENGTH];
    bool auto_release;
    Vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MaterialResourceData;

typedef struct Material
{
    u64 id;
    u32 generation;
    u64 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vec4 diffuse_color;
    TextureMap diffuse_map;
} Material;

typedef struct Geometry
{
    u64 id;
    u32 generation;
    u64 internal_id;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    Material* material;
} Geometry;