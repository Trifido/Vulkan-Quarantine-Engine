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

struct LightData {
    vec4 position;
    vec3 diffuse;
    float constant;
    vec3 specular;
    float linear;
    vec3 spotDirection;
    float quadratic; 
    float spotCutoff;
    float spotExponent;
};

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
	LightData lights[8];
} uboLight;

const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;

layout(std140, set = 0, binding = 3) uniform UniformAnimation
{
	mat4 finalBonesMatrices[200];
} uboAnimation;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in ivec4 inBoneIds; 
layout(location = 6) in vec4 inWeights;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main() 
{ 
    vec4 totalLocalPos = vec4(0.0);
    vec4 totalNormal = vec4(0.0);

    for(int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        vec4 localPosition = uboAnimation.finalBonesMatrices[inBoneIds[i]] * vec4(inPosition, 1.0);
        totalLocalPos += localPosition * inWeights[i];

        vec4 worldNormal = uboAnimation.finalBonesMatrices[inBoneIds[i]] * vec4(inNormal, 0.0);
        totalNormal += worldNormal * inWeights[i]; 
    }

    vs_out.FragPos = vec3(totalLocalPos);
    vs_out.TexCoords = inTexCoord;

    gl_Position = cameraData.viewproj * totalLocalPos;

    vs_out.Normal = totalNormal.xyz;
}
