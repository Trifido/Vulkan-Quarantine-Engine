#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out VS_OUT 
{
    vec3 UVW;
} QE_out;

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
    QE_out.UVW = inPosition.xyz;
    // Convert cubemap coordinates into Vulkan coordinate space
	QE_out.UVW.xy *= -1.0;

    mat4 viewMat = mat4(mat3(constants.model));
    gl_Position = cameraData.proj * viewMat * inPosition;
}
