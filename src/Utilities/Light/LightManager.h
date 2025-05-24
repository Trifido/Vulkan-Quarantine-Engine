#pragma once
#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <unordered_map>
#include <memory>
#include <fstream>
#include <Light/Light.h>
#include <Camera.h>
#include <SwapChainModule.h>
#include <ShaderModule.h>
#include <RenderPassModule.h>
#include <DirectionalLight.h>
#include <SpotLight.h>
#include <PointLight.h>

#include <DescriptorBuffer.h>
#include <PointShadowDescriptorsManager.h>
#include <CSMDescriptorsManager.h>

#include <ShadowPipelineModule.h>

#include <QESingleton.h>
#include <LightDto.h>

class DirectionalLight;
class SpotLight;
class PointLight;

struct LightMap
{
    uint32_t    id;
    float       projected_z;
    float       projected_z_min;
    float       projected_z_max;
};

bool compareDistance(const LightMap& a, const LightMap& b);

class LightManager : public QESingleton<LightManager>
{
private:
    friend class QESingleton<LightManager>; // Permitir acceso al constructor

    const size_t MAX_NUM_LIGHT = 64;
    const uint32_t BIN_SLICES = 16;
    const uint32_t NUM_WORDS = (uint32_t)(MAX_NUM_LIGHT + 31) / 32;
    const uint32_t MAX_NUM_TILES = 240 * 135;

    DeviceModule* deviceModule = nullptr;
    SwapChainModule* swapChainModule = nullptr;
    QECamera* camera = nullptr;

    uint32_t currentNumLights = 0;
    std::unordered_map<std::string, std::shared_ptr<Light>> _lights;
    std::shared_ptr<LightManagerUniform> lightManagerUniform;
    std::vector<LightUniform> lightBuffer;
    std::vector<LightMap> sortedLight;
    std::vector<uint32_t> lights_bin;
    std::vector<uint32_t> lights_index;
    std::vector<uint32_t> light_tiles_bits;

    RenderPassModule* renderPassModule = nullptr;

public:
    std::shared_ptr<UniformBufferObject>    lightUBO;
    std::shared_ptr<UniformBufferObject>    lightSSBO;
    VkDeviceSize                            lightSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightIndexSSBO;
    VkDeviceSize                            lightIndexSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightTilesSSBO;
    VkDeviceSize                            lightTilesSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightBinSSBO;
    VkDeviceSize                            lightBinSSBOSize;

    std::vector<std::shared_ptr<DirectionalLight>> DirLights;
    std::vector<std::shared_ptr<SpotLight>> SpotLights;
    std::vector<std::shared_ptr<PointLight>> PointLights;

    std::shared_ptr<PointShadowDescriptorsManager> PointShadowDescritors;
    std::shared_ptr<CSMDescriptorsManager> CSMDescritors;
    std::shared_ptr<ShadowPipelineModule> CSMPipelineModule;
    std::shared_ptr<ShadowPipelineModule> OmniShadowPipelineModule;

    std::shared_ptr<ShaderModule> CSMShaderModule;
    std::shared_ptr<ShaderModule> OmniShadowShaderModule;

private:
    void AddLight(std::shared_ptr<Light> light_ptr, std::string& name);
    void SortingLights();
    void ComputeLightsLUT();
    void ComputeLightTiles();
    void UpdateUniform();

public:
    LightManager();
    void AddDirShadowMapShader(std::shared_ptr<ShaderModule> shadow_mapping_shader);
    void AddOmniShadowMapShader(std::shared_ptr<ShaderModule> omni_shadow_mapping_shader);
    void CreateLight(LightType type, std::string name);
    void LoadLightDtos(const std::vector <LightDto>& lightDtos);
    static std::vector <LightDto> GetLightDtos(std::ifstream& file);
    void SaveLights(std::ofstream& file);
    std::shared_ptr<Light> GetLight(std::string name);
    void InitializeShadowMaps();
    void Update();
    void UpdateUBOLight();
    void UpdateCSMLights();
    void CleanLightUBO();
    void CleanLastResources();
    void CleanShadowMapResources();
    void SetCamera(QECamera* camera_ptr);
};

#endif
