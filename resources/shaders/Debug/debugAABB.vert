#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(location = 0) in vec4 inPosition;

void main() 
{
    gl_Position = cameraData.viewproj * constants.model * inPosition;
}
