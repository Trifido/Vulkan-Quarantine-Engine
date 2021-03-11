#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 0) out vec3 fragColour;

layout(binding = 0) uniform UboViewProjection
{
	mat4 projection;
	mat4 view;
} uboViewProjection;

layout(binding = 1) uniform UboModel
{
	mat4 model;
} uboModel;

void main()
{
	gl_Position = uboViewProjection.projection * uboViewProjection.view * uboModel.model * vec4(pos, 1.0);
	fragColour = col;
}