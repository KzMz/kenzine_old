{
    "resource": 
    {
        "type": "shader",
        "name": "Shader.Builtin.Material",
        "version": "1.0"
    },
    "renderpass": "Renderpass.Builtin.World",
    "stages": 
    [
        {
            "stage": "vertex",
            "file": "shaders/Builtin.MaterialShader.vert.spv"
        },
        {
            "stage": "fragment",
            "file": "shaders/Builtin.MaterialShader.frag.spv"
        } 
    ],
    "use_instances": true,
    "use_local": true,

    "attributes": 
    [
        {
            "type": "vec3",
            "name": "in_position"
        },
        {
            "type": "vec3",
            "name": "in_normal"
        },
        {
            "type": "vec2",
            "name": "in_texcoord"
        },
        {
            "type": "vec4",
            "name": "in_color"
        },
        {
            "type": "vec4",
            "name": "in_tangent"
        }
    ],

    "uniforms": 
    [
        {
            "type": "mat4",
            "scope": "global",
            "name": "projection"
        },
        {
            "type": "mat4",
            "scope": "global",
            "name": "view"
        },
        {
            "type": "vec4",
            "scope": "global",
            "name": "ambient_color"
        },
        {
            "type": "vec3",
            "scope": "global",
            "name": "view_position"
        },
        {
            "type": "u32",
            "scope": "global",
            "name": "mode"
        },
        {
            "type": "vec4",
            "scope": "instance",
            "name": "diffuse_color"
        },
        {
            "type": "sampler",
            "scope": "instance",
            "name": "diffuse_texture"
        },
        {
            "type": "sampler",
            "scope": "instance",
            "name": "specular_texture"
        },
        {
            "type": "sampler",
            "scope": "instance",
            "name": "normal_texture"
        },
        {
            "type": "f32",
            "scope": "instance",
            "name": "brightness"
        },
        {
            "type": "mat4",
            "scope": "local",
            "name": "model"
        }
    ]
}