#pragma once
#include "lib/math/math_defines.h"

#define RESOURCE_VERSION_MAX_LENGTH 8
#define RESOURCE_CUSTOM_TYPE_MAX_LENGTH 256

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

typedef struct ResourceMetadata
{
    ResourceType type;
    char name[TEXTURE_NAME_MAX_LENGTH];
    char version[RESOURCE_VERSION_MAX_LENGTH];
    char custom_type[RESOURCE_CUSTOM_TYPE_MAX_LENGTH];
} ResourceMetadata;

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

typedef enum MaterialType
{
    MATERIAL_TYPE_WORLD,
    MATERIAL_TYPE_UI
} MaterialType;

typedef struct MaterialResourceData
{
    char name[MATERIAL_NAME_MAX_LENGTH];
    MaterialType type;
    bool auto_release;
    Vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MaterialResourceData;

typedef struct Material
{
    u64 id;
    u32 generation;
    u64 internal_id;
    MaterialType type;
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