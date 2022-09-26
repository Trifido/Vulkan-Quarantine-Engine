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
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    mat3 TBN;
    vec2 TexCoords;
} vs_out;


void main() {
    mat3 normalMatrix = transpose(inverse(mat3(constants.model)));
    vs_out.FragPos = vec3(constants.model * vec4(inPosition, 1.0));
    vs_out.Normal = mat3(constants.model) * inNormal;
    vs_out.TexCoords = inTexCoord;

    vec3 T = normalize(vec3(constants.model * vec4(inTangent,   0.0)));
    vec3 B = normalize(vec3(constants.model * vec4(inBitangent, 0.0)));
    vec3 N = normalize(vec3(constants.model * vec4(inNormal,    0.0)));

    mat3 tTBN = transpose(mat3(T, B, N));
    mat3 TBN = mat3(T, B, N);

    vs_out.TBN = TBN;

    vs_out.TangentViewPos = tTBN * cameraData.position;
    vs_out.TangentFragPos = tTBN * vs_out.FragPos; 

    gl_Position = cameraData.viewproj * vec4(vs_out.FragPos, 1.0);
}
