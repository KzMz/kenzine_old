#pragma once
#include "lib/math/math_defines.h"
#include "core/input/input_defines.h"
#include "lib/containers/hash_table.h"

#define RESOURCE_VERSION_MAX_LENGTH 8
#define RESOURCE_CUSTOM_TYPE_MAX_LENGTH 256

#define TEXTURE_NAME_MAX_LENGTH 512
#define MATERIAL_NAME_MAX_LENGTH 256
#define GEOMETRY_NAME_MAX_LENGTH 256
#define SHADER_NAME_MAX_LENGTH 512
#define DEVICE_NAME_MAX_LENGTH 256
#define DEVICE_KEY_NAME_MAX_LENGTH 50

#define MAX_IMAGE_PATH_LENGTH 512

typedef enum ResourceType
{
    RESOURCE_TYPE_TEXT,
    RESOURCE_TYPE_BINARY,
    RESOURCE_TYPE_IMAGE,
    RESOURCE_TYPE_MATERIAL,
    RESOURCE_TYPE_STATIC_MESH,
    RESOURCE_TYPE_SHADER,
    RESOURCE_TYPE_DEVICE,
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
    TEXTURE_USE_SPECULAR,
    TEXTURE_USE_NORMAL
} TextureUsage;

typedef struct TextureMap
{
    Texture* texture;
    TextureUsage usage;
} TextureMap;

typedef struct MaterialResourceData
{
    char name[MATERIAL_NAME_MAX_LENGTH];
    char shader_name[SHADER_NAME_MAX_LENGTH];
    bool auto_release;
    Vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
    char specular_map_name[TEXTURE_NAME_MAX_LENGTH];
    char normal_map_name[TEXTURE_NAME_MAX_LENGTH];
    f32 brightness;
} MaterialResourceData;

typedef struct Material
{
    u64 id;
    u32 generation;
    u64 internal_id;
    char name[MATERIAL_NAME_MAX_LENGTH];
    Vec4 diffuse_color;
    TextureMap diffuse_map;
    TextureMap specular_map;
    TextureMap normal_map;
    f32 brightness;
    u64 shader_id;
} Material;

typedef struct Geometry
{
    u64 id;
    u32 generation;
    u64 internal_id;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    Material* material;
} Geometry;

typedef enum ShaderStage
{
    SHADER_STAGE_VERTEX = 0x01,
    SHADER_STAGE_GEOMETRY = 0x02,
    SHADER_STAGE_FRAGMENT = 0x04,
    SHADER_STAGE_COMPUTE = 0x08
} ShaderStage;

typedef enum ShaderAttributeType {
    SHADER_ATTRIB_TYPE_FLOAT32 = 0U,
    SHADER_ATTRIB_TYPE_FLOAT32_2,
    SHADER_ATTRIB_TYPE_FLOAT32_3,
    SHADER_ATTRIB_TYPE_FLOAT32_4,
    SHADER_ATTRIB_TYPE_MATRIX_4,
    SHADER_ATTRIB_TYPE_INT8,
    SHADER_ATTRIB_TYPE_UINT8,
    SHADER_ATTRIB_TYPE_INT16,
    SHADER_ATTRIB_TYPE_UINT16,
    SHADER_ATTRIB_TYPE_INT32,
    SHADER_ATTRIB_TYPE_UINT32,
} ShaderAttributeType;

typedef enum ShaderUniformType {
    SHADER_UNIFORM_TYPE_FLOAT32 = 0U,
    SHADER_UNIFORM_TYPE_FLOAT32_2,
    SHADER_UNIFORM_TYPE_FLOAT32_3,
    SHADER_UNIFORM_TYPE_FLOAT32_4,
    SHADER_UNIFORM_TYPE_INT8,
    SHADER_UNIFORM_TYPE_UINT8,
    SHADER_UNIFORM_TYPE_INT16,
    SHADER_UNIFORM_TYPE_UINT16,
    SHADER_UNIFORM_TYPE_INT32,
    SHADER_UNIFORM_TYPE_UINT32,
    SHADER_UNIFORM_TYPE_MATRIX_4,
    SHADER_UNIFORM_TYPE_SAMPLER,
    SHADER_UNIFORM_TYPE_CUSTOM = 255U
} ShaderUniformType;

typedef enum ShaderScope {
    SHADER_SCOPE_GLOBAL = 0,
    SHADER_SCOPE_INSTANCE,
    SHADER_SCOPE_LOCAL
} ShaderScope;

typedef struct ShaderAttributeConfig
{
    u8 name_length;
    char* name;
    u8 size;
    ShaderAttributeType type;
} ShaderAttributeConfig;

typedef struct ShaderUniformConfig
{
    u8 name_length;
    char* name;
    u8 size;
    u64 location;
    ShaderUniformType type;
    ShaderScope scope;
} ShaderUniformConfig;

typedef struct ShaderConfig
{
    char* name;

    bool use_instances;
    bool use_local;

    u8 attribute_count;
    ShaderAttributeConfig* attributes;

    u8 uniform_count;
    ShaderUniformConfig* uniforms;

    char* renderpass_name;

    u8 stage_count;
    ShaderStage* stages;
    char** stage_names;
    const char** stage_files;
} ShaderConfig;

typedef enum DeviceType
{
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_KEYBOARD,
    DEVICE_TYPE_MOUSE,
    DEVICE_TYPE_GAMEPAD,
} DeviceType;

typedef enum DeviceGamepadType
{
    DEVICE_TYPE_GAMEPAD_NONE,
    DEVICE_TYPE_GAMEPAD_XBOX,
    DEVICE_TYPE_GAMEPAD_DUALSHOCK4,
    DEVICE_TYPE_GAMEPAD_SWITCH,
    DEVICE_TYPE_GAMEPAD_STEAM,
    DEVICE_TYPE_GAMEPAD_GENERIC
} DeviceGamepadType;

typedef struct DeviceInputActionConfig
{
    char action_name[MAX_INPUTACTION_NAME_LENGTH];
    InputActionType action_type;
    InputActionAxisType axis_type;
    char key_name[DEVICE_KEY_NAME_MAX_LENGTH];
    char positive_axis_key_name[DEVICE_KEY_NAME_MAX_LENGTH];
    char negative_axis_key_name[DEVICE_KEY_NAME_MAX_LENGTH];
    char native_axis_key_name[DEVICE_KEY_NAME_MAX_LENGTH];
} DeviceInputActionConfig;

typedef struct DeviceConfig
{
    char* name;
    i32 sub_id;
    DeviceType type;
    DeviceGamepadType gamepad_type;

    HashTable keys;

    u8 actions_count;
    DeviceInputActionConfig* actions;
} DeviceConfig;