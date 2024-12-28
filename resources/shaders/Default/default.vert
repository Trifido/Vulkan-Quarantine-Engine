#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Pos;
    vec3 ViewPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
} vs_out;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

void main() {
    vs_out.FragPos = (constants.model * inPosition).xyz;
    vs_out.Pos = inPosition.xyz;
    vs_out.ViewPos = (cameraData.view * inPosition).xyz;
    vs_out.TexCoords = inTexCoord;
    vs_out.Normal = inNormal.xyz;

    vec3 T = normalize(constants.model * inTangent).xyz;
    vec3 N = normalize(constants.model * inNormal).xyz;
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    vs_out.TBN = mat3(T, B, N);

    gl_Position = cameraData.viewproj * vec4(vs_out.FragPos, 1.0);
}
