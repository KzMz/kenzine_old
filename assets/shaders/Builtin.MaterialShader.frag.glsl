#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform local_uniform_object_
{
    vec4 diffuse_color;
} local_uniform_object;

struct directional_light 
{
    vec3 direction;
    vec4 color;
};

directional_light dir_light = 
{
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

layout(set = 1, binding = 1) uniform sampler2D diffuse_sampler;

layout(location = 1) in struct dto 
{
    vec4 ambient;
    vec2 texcoord;
    vec3 normal;
} in_dto;

vec4 calculate_directional_light(directional_light light, vec3 normal);

void main()
{
    out_color = calculate_directional_light(dir_light, in_dto.normal);
}

vec4 calculate_directional_light(directional_light light, vec3 normal)
{
    float diffuse_factor = max(dot(normal, -light.direction), 0.0);

    vec4 diff_sampler = texture(diffuse_sampler, in_dto.texcoord);
    vec4 ambient = vec4(vec3(in_dto.ambient * local_uniform_object.diffuse_color), diff_sampler.a);
    vec4 diffuse = vec4(vec3(light.color * diffuse_factor), diff_sampler.a);

    diffuse *= diff_sampler;
    ambient *= diff_sampler;

    return (ambient + diffuse);
}