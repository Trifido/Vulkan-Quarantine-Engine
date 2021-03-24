#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColour;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = subpassLoad(inputColour).rgba;
}