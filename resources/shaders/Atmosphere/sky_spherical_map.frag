#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT 
{
    vec3 UVW;
} QE_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D environmentMap;

vec2 sphericalToUV(vec3 dir)
{
    float u = 0.5 + atan(dir.z, dir.x) / (2.0 * 3.14159265359); // Longitud
    float v = 0.5 - asin(dir.y) / 3.14159265359;               // Latitud
    return vec2(u, v);
}

void main()
{
    vec3 normalizedDir = normalize(QE_in.UVW);
    vec2 uv = sphericalToUV(normalizedDir);
    outColor = texture(environmentMap, uv);
}