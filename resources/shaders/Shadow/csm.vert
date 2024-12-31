#version 450

layout (location = 0) in vec4 inPosition;

// todo: pass via specialization constant
#define CSM_COUNT 4

layout(push_constant) uniform PushConsts 
{
	mat4 model;
	uint cascadeIndex;
} constants;

layout (set = 0, binding = 0) uniform CSMUniform
{
	mat4[CSM_COUNT] cascadeViewProj;
} csm;

void main()
{
	gl_Position =  csm.cascadeViewProj[constants.cascadeIndex] * constants.model * vec4(inPosition.xyz, 1.0);
}