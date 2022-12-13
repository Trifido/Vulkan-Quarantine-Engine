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
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 TangentLightPos[8];
    vec2 TexCoords;
} vs_out;

void main() 
{
    mat4 BoneTransform = uboAnimation.finalBonesMatrices[inBoneIds[0]] * inWeights[0];
    BoneTransform += uboAnimation.finalBonesMatrices[inBoneIds[1]] * inWeights[1];
    BoneTransform += uboAnimation.finalBonesMatrices[inBoneIds[2]] * inWeights[2];
    BoneTransform += uboAnimation.finalBonesMatrices[inBoneIds[3]] * inWeights[3];    

    vec4 tBonePosition = BoneTransform * vec4(inPosition, 1.0);

    vs_out.FragPos = vec3(constants.model * tBonePosition);
    vs_out.TexCoords = inTexCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(BoneTransform)));
    vs_out.Normal = normalMatrix * inNormal;

    vec3 T = normalize(normalMatrix * inTangent);
    vec3 N = normalize(normalMatrix * inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));   

    vs_out.TangentViewPos = TBN * cameraData.position;
    vs_out.TangentFragPos = TBN * vs_out.FragPos; 

    for(int i = 0; i < uboLight.numLights; i++)
    {
        vs_out.TangentLightPos[i] = TBN * vec3(uboLight.lights[i].position);
    }

    gl_Position = cameraData.viewproj * vec4(vs_out.FragPos, 1.0);
}
