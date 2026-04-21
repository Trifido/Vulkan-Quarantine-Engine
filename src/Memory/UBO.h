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

struct alignas(16) UniformCamera
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::mat4 ViewProjection;
    glm::mat4 InvView;
    glm::mat4 InvProjection;
    glm::mat4 InvViewProjection;
    glm::vec4 Position;
    glm::vec4 Params; // x=near, y=far, z=fovY, w=aspect
    glm::vec4 FrustumPlanes[6];
};

struct ScreenResolutionUniform
{
    glm::vec2 resolution;
};

struct alignas(16) ScreenDataUniform
{
    glm::uvec2 tilePixelSize;
    glm::uvec2 tileCount;
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
    int idxMetallic;
    int idxRoughness;
    int idxAO;

    float Opacity;
    float BumpScaling;
    float Reflectivity;
    float Refractivity;
    float Shininess;
    float Shininess_Strength;
    float Metallic;
    float Roughness;
    float AO;
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

struct SunUniform
{
    glm::vec3 Direction;
    float Intensity;
};

struct alignas(16) AtmosphereUniform
{
    // xyz = scattering, w = scale height
    glm::vec4 RayleighScattering_Height;

    // xyz = scattering, w = scale height
    glm::vec4 MieScattering_Height;

    // xyz = absorption, w = density
    glm::vec4 OzoneAbsorption_Density;

    // xyz = sun color, w = sun intensity multiplier
    glm::vec4 SunColor_Intensity;

    // x = planet radius
    // y = atmosphere radius
    // z = mie anisotropy
    // w = exposure
    glm::vec4 Planet_Atmosphere_G_Exposure;

    // x = sky tint
    // y = horizon softness
    // z = multi scattering factor
    // w = sun disk size
    glm::vec4 Sky_Horizon_Multi_SunDisk;

    // x = sun disk intensity
    // y = sun glow
    // z,w = reserved
    glm::vec4 SunDiskIntensity_Glow_Padding;

    AtmosphereUniform()
        : RayleighScattering_Height(5.802f, 13.558f, 33.100f, 8000.0f),
        MieScattering_Height(3.996f, 3.996f, 3.996f, 1200.0f),
        OzoneAbsorption_Density(0.650f, 1.881f, 0.085f, 1.0f),
        SunColor_Intensity(1.0f, 0.98f, 0.92f, 1.0f),
        Planet_Atmosphere_G_Exposure(6360000.0f, 6460000.0f, 0.8f, 10.0f),
        Sky_Horizon_Multi_SunDisk(1.0f, 1.0f, 1.0f, 1.0f),
        SunDiskIntensity_Glow_Padding(1.0f, 1.0f, 0.0f, 0.0f)
    {
    }
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

struct CascadeSplitUniform
{
    float cascadeSplits[10][CSM_NUM];
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
