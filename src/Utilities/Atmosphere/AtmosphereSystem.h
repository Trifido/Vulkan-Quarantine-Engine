#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <ShaderModule.h>
#include <CustomTexture.h>
#include <QECamera.h>
#include <LightManager.h>
#include <QESingleton.h>
#include <Compute/ComputeNodeManager.h>
#include <AtmosphereDto.h>
#include <SunLight.h>
#include <AtmosphereType.h>

using namespace std;

class AtmosphereSystem : public QESingleton<AtmosphereSystem>, public SerializableComponent
{
private:
    const std::string SUN_NAME = "QESunLight";
    std::shared_ptr<QESunLight> sunLight;
    glm::vec3 sunDirection;
    float sunIntensity;

public:
    bool IsInitialized;

private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* deviceModule;
    QECamera* camera = nullptr;
    LightManager* lightManager = nullptr;
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapChainModule;

    std::shared_ptr<CustomTexture> outputTexture;
    std::shared_ptr<UniformBufferObject> resolutionUBO = nullptr;

    AtmosphereType atmosphereType;

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
    void SetUpResources(QECamera* cameraPtr);

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    const std::string& getTypeName() const override {
        static const std::string name = "AtmosphereSystem";
        return name;
    }
    QEMetaType* meta() const override {
        return nullptr; // o un meta vacío si no usas campos “normales”
    }

    void LoadAtmosphereDto(AtmosphereDto atmosphereDto, QECamera* cameraPtr);
    AtmosphereDto CreateAtmosphereDto();
    void AddTextureResources(const string* texturePaths, uint32_t numTextures);
    void InitializeAtmosphere(QECamera* cameraPtr);
    void InitializeAtmosphere(AtmosphereType type, const string* texturePaths, uint32_t numTextures, QECamera* cameraPtr);
    void SetCamera(QECamera* cameraPtr);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx);
    void Cleanup();
    void CleanLastResources();
    void UpdateSun();
    void UpdateAtmopshereResolution();
    YAML::Node serialize() const;
    void deserialize(const YAML::Node& node);
};

#endif // !ATMOSPHERE_SYSTEM_H


