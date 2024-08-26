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
            "type": "vec2",
            "name": "in_texcoord"
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
            "scope": "instance",
            "name": "diffuse_color"
        },
        {
            "type": "sampler",
            "scope": "instance",
            "name": "diffuse_texture"
        },
        {
            "type": "mat4",
            "scope": "local",
            "name": "model"
        }
    ]
}