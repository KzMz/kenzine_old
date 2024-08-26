#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(set = 0, binding = 0) uniform global_uniform_
{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;
} global_uniform;

layout(push_constant) uniform push_constants_
{
    // total 128 bytes
    mat4 model; // 64 bytes
} push_constants;

layout(location = 0) out int out_mode;

layout(location = 1) out struct dto 
{
    vec4 ambient;
    vec2 texcoord;
    vec3 normal;
} out_dto;

void main()
{
    out_dto.texcoord = in_texcoord;
    out_dto.ambient = global_uniform.ambient_color;
    out_dto.normal = mat3(push_constants.model) * in_normal;
    gl_Position = global_uniform.projection * global_uniform.view * push_constants.model * vec4(in_position, 1.0);
}