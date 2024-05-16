#version 450

layout(location = 0) out float color;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 inLightPosition;

void main() 
{	
	vec3 lightVec = inPosition.xyz - inLightPosition;
	color = length(lightVec);
}