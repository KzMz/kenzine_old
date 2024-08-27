#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in vec4 in_color;
layout(location = 4) in vec4 in_tangent;

layout(set = 0, binding = 0) uniform global_uniform_
{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;
    vec3 view_position;
    int mode;
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
    vec3 view_position;
    vec3 frag_position;
    vec4 color;
    vec4 tangent;
} out_dto;

void main()
{
    out_dto.texcoord = in_texcoord;
    out_dto.color = in_color;
    out_dto.frag_position = vec3(push_constants.model * vec4(in_position, 1.0));
    
    mat3 model3 = mat3(push_constants.model);
    out_dto.normal = model3 * in_normal;
    out_dto.tangent = vec4(normalize(model3 * in_tangent.xyz), in_tangent.w);

    out_dto.ambient = global_uniform.ambient_color;
    out_dto.view_position = global_uniform.view_position;
    gl_Position = global_uniform.projection * global_uniform.view * push_constants.model * vec4(in_position, 1.0);

    out_mode = global_uniform.mode;
}