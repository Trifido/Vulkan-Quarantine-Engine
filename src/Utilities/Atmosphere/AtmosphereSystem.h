#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <ShaderModule.h>
#include <CustomTexture.h>
#include <LightManager.h>
#include <QESingleton.h>
#include <Compute/ComputeNodeManager.h>
#include <AtmosphereDto.h>
#include <SunLight.h>
#include <AtmosphereType.h>

class AtmosphereSystem : public QESingleton<AtmosphereSystem>, public SerializableComponent
{
private:
    const std::string SUN_NAME = "QESunLight";
    const std::string TLUT_NODE_NAME = "transmittance_lut";
    const std::string MSLUT_NODE_NAME = "multi_scattering_lut";
    const std::string SVLUT_NODE_NAME = "sky_view_lut";
    std::shared_ptr<QESunLight> sunLight;
    glm::vec3 sunDirection;
    float sunIntensity;

private:
    AtmosphereUniform atmosphereData{};

    void ApplyDtoToAtmosphereData(const AtmosphereDto& dto);
    AtmosphereDto BuildDtoFromAtmosphereData() const;
    void UpdateAtmosphereUBO();
    void MarkAtmosphereLutsDirty();

public:
    bool IsInitialized;

private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* deviceModule;
    LightManager* lightManager = nullptr;
    ComputeNodeManager* computeNodeManager;
    SwapChainModule* swapChainModule;

    std::shared_ptr<CustomTexture> outputTexture;
    std::shared_ptr<UniformBufferObject> resolutionUBO = nullptr;
    std::shared_ptr<UniformBufferObject> atmosphereUBO = nullptr;

    AtmosphereType atmosphereType;

    // Skybox shader
    const std::vector<std::string> shaderPaths = {
        "Atmosphere/skybox_cubemap_vert.spv",
        "Atmosphere/sky_spherical_map_vert.spv",
        "Atmosphere/atmosphere_vert.spv",
        "Atmosphere/skybox_cubemap_frag.spv",
        "Atmosphere/sky_spherical_map_frag.spv",
        "Atmosphere/atmosphere_frag.spv",
    };

    std::shared_ptr<ShaderModule> environment_shader;

    std::shared_ptr<ComputeNode> TLUT_ComputeNode;
    std::shared_ptr<ComputeNode> MSLUT_ComputeNode;
    std::shared_ptr<ComputeNode> SVLUT_ComputeNode;

    // Mesh
    std::shared_ptr<QEGeometryComponent> _Mesh = nullptr;

    // Descriptor set
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Cubemap resources
    std::shared_ptr<CustomTexture> environmentTexture = nullptr;

    std::vector<VkDescriptorBufferInfo> buffersInfo;
    VkDescriptorImageInfo imageInfo_1;
    VkDescriptorImageInfo imageInfo_2;

private:
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    std::string GetAbsolutePath(std::string relativePath, std::string filename);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, uint32_t idBuffer, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);
    void SetSamplerDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, std::shared_ptr<CustomTexture> texture, VkDescriptorImageInfo& imageInfo);
    void SetUpResources();

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    const std::string& getTypeName() const override {
        static const std::string name = "AtmosphereSystem";
        return name;
    }
    QEMetaType* meta() const override {
        return nullptr;
    }

    void LoadAtmosphereDto(AtmosphereDto atmosphereDto);
    void InitializeAtmosphereResources();
    AtmosphereDto CreateAtmosphereDto();
    void AddTextureResources(const std::string* texturePaths, uint32_t numTextures);
    void InitializeAtmosphere();
    void InitializeAtmosphere(AtmosphereType type, const std::string* texturePaths, uint32_t numTextures);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx);

    void ResetSceneState();
    void Cleanup();
    void CleanLastResources();

    void UpdateSun();
    void UpdateAtmopshereResolution();
    void UpdatePerFrame(uint32_t frame);

    glm::vec3 GetSunEulerDegrees() const;
    void SetSunEulerDegrees(const glm::vec3& eulerDeg);

    float GetSunBaseIntensity() const;
    void SetSunBaseIntensity(float intensity);

    AtmosphereDto GetEditableAtmosphereDto();
    void ApplyEditableAtmosphereDto(const AtmosphereDto& dto, bool rebuildLuts);

    std::shared_ptr<QESunLight> GetSunLight() const { return this->sunLight; }
};

#endif // !ATMOSPHERE_SYSTEM_H


