#version 450

layout (location = 0) in vec4 inPosition;

// todo: pass via specialization constant
#define SHADOW_MAP_CASCADE_COUNT 4

layout(push_constant) uniform PushConsts 
{
	mat4 model;
	uint cascadeIndex;
} constants;

layout (set = 0, binding = 0) uniform CSMUniform
{
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

void main()
{
	gl_Position =  ubo.cascadeViewProj[constants.cascadeIndex] * constants.model * vec4(inPosition.xyz, 1.0);
}