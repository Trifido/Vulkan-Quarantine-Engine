#pragma once
#ifndef UBO_H
#define UBO_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/matrix.hpp>
#include <DeviceModule.h>

struct LightUniform
{
    glm::vec3 position;
    uint32_t lightType;
    glm::vec3 diffuse;
    float constant;
    glm::vec3 specular;
    float linear;
    glm::vec3 direction;
    float quadratic;
    float cutOff;
    float outerCutoff;
    float radius;
    uint32_t idxShadowMap;
};

struct LightManagerUniform
{
    uint32_t numLights;
};

struct CameraUniform
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewproj;
    glm::vec4 position;
    alignas(16) glm::vec4 frustumPlanes[6];
};

struct MaterialUniform
{
    glm::vec4 Diffuse;
    glm::vec4 Ambient;
    glm::vec4 Specular;
    glm::vec4 Emissive;
    glm::vec4 Transparent;
    glm::vec4 Reflective;

    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;

    float Opacity;
    float BumpScaling;
    float Reflectivity;
    float Refractivity;
    float Shininess;
    float Shininess_Strength;
};

struct ParticleSystemUniform
{
    glm::vec4  initialColor;
    float particleLifeTime;
    float particleSystemDuration;
    float particlePerFrame;
    float gravity;
    float emissionAngle;
    float emissionRadius;
    float speed;
    float angularSpeed;
    float initAngle;
    float initSize;
    float auxData;
    uint32_t  maxParticles;
};

struct ParticleTextureParamsUniform
{
    float numCols;
    float numRows;
    float totalSprites;
    float auxiliarData;
};

struct NewParticleUniform
{
    uint32_t newParticles;
    uint32_t frameCount;
};

struct AnimationUniform
{
    alignas(16) glm::mat4 finalBonesMatrices[200];
};

struct PushConstantStruct
{
    glm::mat4 model;
};

struct PushConstantOmniShadowStruct
{
    glm::mat4 model;
    glm::mat4 lightModel;
    glm::mat4 view;
};

struct PushConstantCSMStruct
{
    glm::mat4 model;
    uint32_t cascadeIndex;
};

struct PushConstantViewStruct
{
    glm::mat4 view;
};

struct DeltaTimeUniform
{
    float deltaTime;
};

struct OmniShadowUniform
{
    glm::mat4 projection;
    glm::vec4 lightPos;
};

const int CSM_NUM = 4;

struct CSMUniform
{
    glm::mat4 cascadeViewProj[CSM_NUM];
};

class UniformBufferObject
{
public:
    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VkDeviceMemory>     uniformBuffersMemory;

    void CreateUniformBuffer(VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device);
    void CreateSSBO(VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device);
    void FillSSBO(VkBuffer stagingBuffer, VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device);
};

#endif
