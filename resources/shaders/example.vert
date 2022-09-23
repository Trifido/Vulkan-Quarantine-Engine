#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
//layout(location = 3) in vec3 color;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;


void main() {
    gl_Position = cameraData.viewproj * constants.model * vec4(inPosition, 1.0);
    //gl_Position =  constants.model * vec4(inPosition, 1.0);
    fragColor = inNormal;
    fragTexCoord = inTexCoord;
}
