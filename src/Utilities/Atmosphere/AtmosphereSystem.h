#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <ShaderModule.h>
#include <CustomTexture.h>
#include <Camera.h>
#include <LightManager.h>
#include <QESingleton.h>
#include <Compute/ComputeNodeManager.h>
#include <AtmosphereDto.h>
#include <SunLight.h>

using namespace std;

class AtmosphereSystem : public QESingleton<AtmosphereSystem>
{
private:
    const std::string SUN_NAME = "QESunLight";
    std::shared_ptr<SunLight> sunLight;

public:
    enum ENVIRONMENT_TYPE
    {
        CUBEMAP = 0,
        SPHERICALMAP = 1,
        PHYSICALLY_BASED_SKY = 2,
    };

    bool IsInitialized = false;

private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* deviceModule;
    Camera* camera = nullptr;
    LightManager* lightManager = nullptr;
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapChainModule;

    std::shared_ptr<CustomTexture> outputTexture;
    std::shared_ptr<UniformBufferObject> resolutionUBO = nullptr;

    // Environment type
    ENVIRONMENT_TYPE environmentType;

    // Skybox shader
    const vector<string> shaderPaths = {
        "Atmosphere/skybox_cubemap_vert.spv",
        "Atmosphere/sky_spherical_map_vert.spv",
        "Atmosphere/atmosphere_vert.spv",
        "Atmosphere/skybox_cubemap_frag.spv",
        "Atmosphere/sky_spherical_map_frag.spv",
        "Atmosphere/atmosphere_frag.spv",
    };

    shared_ptr<ShaderModule> environment_shader;

    shared_ptr<ComputeNode> TLUT_ComputeNode;
    shared_ptr<ComputeNode> MSLUT_ComputeNode;
    shared_ptr<ComputeNode> SVLUT_ComputeNode;

    // Mesh
    shared_ptr<QEGeometryComponent> _Mesh = nullptr;

    // Descriptor set
    vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Cubemap resources
    shared_ptr<CustomTexture> environmentTexture = nullptr;

    std::vector<VkDescriptorBufferInfo> buffersInfo;
    VkDescriptorImageInfo imageInfo_1;
    VkDescriptorImageInfo imageInfo_2;

private:
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    string GetAbsolutePath(string relativePath, string filename);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, uint32_t idBuffer, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);
    void SetSamplerDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, std::shared_ptr<CustomTexture> texture, VkDescriptorImageInfo& imageInfo);
    void SetUpResources(Camera* cameraPtr);

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    void LoadAtmosphereDto(AtmosphereDto atmosphereDto, Camera* cameraPtr);
    AtmosphereDto CreateAtmosphereDto();
    void AddTextureResources(const string* texturePaths, uint32_t numTextures);
    void InitializeAtmosphere(Camera* cameraPtr);
    void InitializeAtmosphere(ENVIRONMENT_TYPE type, const string* texturePaths, uint32_t numTextures, Camera* cameraPtr);
    void SetCamera(Camera* cameraPtr);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx);
    void Cleanup();
    void CleanLastResources();
    void UpdateSun();
    void UpdateAtmopshereResolution();
};

#endif // !ATMOSPHERE_SYSTEM_H


