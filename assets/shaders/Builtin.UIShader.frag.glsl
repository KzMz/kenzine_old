#version 450

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform local_uniform_object_
{
    vec4 diffuse_color;
} local_uniform_object;

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

layout(location = 1) in struct dto 
{
    vec2 texcoord;
} in_dto;

void main()
{
    out_color = local_uniform_object.diffuse_color * texture(diffuse_sampler, in_dto.texcoord);
}