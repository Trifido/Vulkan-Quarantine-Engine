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

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

void main() 
{
    fragColor = inColor.rgb;
    gl_Position = cameraData.viewproj * inPosition;
}
