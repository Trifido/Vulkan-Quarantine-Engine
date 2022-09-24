#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;


void main() {
    mat3 normalMatrix = transpose(inverse(mat3(constants.model)));
    vs_out.FragPos = vec3(constants.model * vec4(inPosition, 1.0));
    vs_out.Normal = normalMatrix * inNormal;
    vs_out.TexCoords = inTexCoord;

    gl_Position = cameraData.viewproj * constants.model * vec4(inPosition, 1.0);
}
