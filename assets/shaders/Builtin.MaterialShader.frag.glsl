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

struct point_light
{
    vec3 position;
    vec4 color;
    float constant; // usually 1
    float linear;
    float quadratic;
};

directional_light dir_light = 
{
    vec3(-0.57735, -0.57735, -0.57735),
    vec4(0.8, 0.8, 0.8, 1.0)
};

point_light p0 = 
{
    vec3(-5.5, 0.0, -5.5),
    vec4(0.0, 1.0, 0.0, 1.0),
    1.0,
    0.35,
    0.44
};

point_light p1 = 
{
    vec3(5.5, 0.0, -5.5),
    vec4(1.0, 0.0, 0.0, 1.0),
    1.0,
    0.35,
    0.44
};

const int SAMPLER_DIFFUSE = 0;
const int SAMPLER_SPECULAR = 1;
const int SAMPLER_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

const int MODE_DEFAULT = 0;
const int MODE_LIGHTING = 1;
const int MODE_NORMALS = 2;
layout(location = 0) flat in int in_mode;
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
vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main()
{
    vec3 normal = in_dto.normal;
    vec3 tangent = in_dto.tangent.xyz;
    tangent = (tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(in_dto.normal, in_dto.tangent.xyz) * in_dto.tangent.w;
    TBN = mat3(tangent, bitangent, normal);

    vec3 local_normal = 2.0 * texture(samplers[SAMPLER_NORMAL], in_dto.texcoord).rgb - 1.0;
    normal = normalize(TBN * local_normal);

    if (in_mode == MODE_DEFAULT || in_mode == MODE_LIGHTING)
    {
        vec3 view_direction = normalize(in_dto.view_position - in_dto.frag_position);   

        out_color = calculate_directional_light(dir_light, normal, view_direction);
        out_color += calculate_point_light(p0, normal, in_dto.frag_position, view_direction);
        out_color += calculate_point_light(p1, normal, in_dto.frag_position, view_direction); 
    }
    else if (in_mode == MODE_NORMALS)
    {
        out_color = vec4(abs(normal), 1.0);
    }
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

    if (in_mode == MODE_DEFAULT)
    {
        diffuse *= diff_sampler;
        ambient *= diff_sampler;
        specular *= vec4(texture(samplers[SAMPLER_SPECULAR], in_dto.texcoord).rgb, diffuse.a);
    }

    return (ambient + diffuse + specular);
}

vec4 calculate_point_light(point_light light, vec3 normal, vec3 frag_position, vec3 view_direction)
{
    vec3 light_direction = normalize(light.position - frag_position);
    float diffuse_factor = max(dot(normal, light_direction), 0.0);

    vec3 reflect_direction = reflect(-light_direction, normal);
    float specular_factor = pow(max(dot(view_direction, reflect_direction), 0.0), local_uniform_object.brightness);

    float distance = length(light.position - frag_position);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec4 ambient = in_dto.ambient;
    vec4 diffuse = light.color * diffuse_factor;
    vec4 specular = light.color * specular_factor;

    if (in_mode == MODE_DEFAULT)
    {
        vec4 diff_sampler = texture(samplers[SAMPLER_DIFFUSE], in_dto.texcoord);
        diffuse *= diff_sampler;
        ambient *= diff_sampler;
        specular *= vec4(texture(samplers[SAMPLER_SPECULAR], in_dto.texcoord).rgb, diff_sampler.a);
    }

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}