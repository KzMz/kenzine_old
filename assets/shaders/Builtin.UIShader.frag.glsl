#version 450

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform local_uniform_object_
{
    vec4 diffuse_color;
} local_uniform_object;

const int SAMPLER_DIFFUSE = 0;
layout(set = 1, binding = 1) uniform sampler2D samplers[1];

layout(location = 1) in struct dto 
{
    vec2 texcoord;
} in_dto;

void main()
{
    out_color = local_uniform_object.diffuse_color * texture(samplers[SAMPLER_DIFFUSE], in_dto.texcoord);
}