#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 inPosition;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outLightPosition;

layout(set = 0, binding = 0) uniform PointLightCameraUniform
{
	mat4 projection;
	vec4 lightPos;
} plData;

layout(std430, push_constant) uniform PushConstants
{
	mat4 model;
	mat4 lightModel;
	mat4 view;
} constants;

void main() 
{
    gl_Position = plData.projection * constants.view * constants.lightModel * inPosition;

    outPosition = constants.model * inPosition;	
	outLightPosition = plData.lightPos.xyz; 
}