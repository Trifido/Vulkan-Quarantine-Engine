#version 450

layout (location = 0) in vec3 inPos;

// todo: pass via specialization constant
#define SHADOW_MAP_CASCADE_COUNT 4

layout(push_constant) uniform PushConsts {
	vec4 position;
	mat4 model;
	uint cascadeIndex;
} constants;

layout (set = 0, binding = 0) uniform CSMUniform {
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProj;
} ubo;

void main()
{
	vec3 pos = inPos + constants.position.xyz;
	gl_Position =  ubo.cascadeViewProj[constants.cascadeIndex] * vec4(pos, 1.0);
}