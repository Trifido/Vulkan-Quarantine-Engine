#version 450
#include "../Includes/QECommon.glsl"

layout (location = 0) in vec4 inPosition;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec2 outTexCoord;

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

layout(set = 1, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

void main()
{
	outTexCoord = inTexCoord;
	gl_Position =  csm.cascadeViewProj[constants.cascadeIndex] * constants.model * vec4(inPosition.xyz, 1.0);
}
