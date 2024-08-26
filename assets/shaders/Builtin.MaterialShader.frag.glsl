#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform local_uniform_object_
{
    vec4 diffuse_color;
    float brightness;
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

const int SAMPLER_DIFFUSE = 0;
const int SAMPLER_SPECULAR = 1;
const int SAMPLER_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

layout(location = 1) in struct dto 
{
    vec4 ambient;
    vec2 texcoord;
    vec3 normal;
    vec3 view_position;
    vec3 frag_position;
    vec4 color;
    vec4 tangent;
} in_dto;

mat3 TBN;

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction);

void main()
{
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent.xyz) * in_dto.tangent.w;
    TBN = mat3(tangent, bitangent, normal);

    vec3 local_normal = 2.0 * texture(samplers[SAMPLER_NORMAL], in_dto.texcoord).rgb - 1.0;
    normal = normalize(TBN * local_normal);

    vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);
    out_color = calculate_directional_light(dir_light, normal, view_direction);
}

vec4 calculate_directional_light(directional_light light, vec3 normal, vec3 view_direction)
{
    float diffuse_factor = max(dot(normal, -light.direction), 0.0);

    vec3 half_direction = normalize(view_direction - light.direction);
    float specular_factor = pow(max(dot(half_direction, normal), 0.0), local_uniform_object.brightness);

    vec4 diff_sampler = texture(samplers[SAMPLER_DIFFUSE], in_dto.texcoord);
    vec4 ambient = vec4(vec3(in_dto.ambient * local_uniform_object.diffuse_color), diff_sampler.a);
    vec4 diffuse = vec4(vec3(light.color * diffuse_factor), diff_sampler.a);
    vec4 specular = vec4(vec3(light.color * specular_factor), diff_sampler.a);

    diffuse *= diff_sampler;
    ambient *= diff_sampler;
    specular *= vec4(texture(samplers[SAMPLER_SPECULAR], in_dto.texcoord).rgb, diffuse.a);

    return (ambient + diffuse + specular);
}