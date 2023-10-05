#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inLifeTime;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec3 inVelocity;
layout(location = 4) in float inSeed;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec3 position;
} cameraData;

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

void main() 
{
    vec4 position = (constants.model * vec4(inPosition, 1.0));
    gl_Position = cameraData.viewproj * position;
    gl_PointSize = 14.0;
    fragColor = inColor;
}
