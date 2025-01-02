#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT 
{
    vec3 UVW;
} QE_in;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube atmosphereCubeMap;

void main()
{
    outColor = texture(atmosphereCubeMap, QE_in.UVW);
}