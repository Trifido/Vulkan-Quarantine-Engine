#pragma once
#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <unordered_map>
#include <memory>
#include <Light/Light.h>
#include <Camera.h>
#include <SwapChainModule.h>
#include <ShaderModule.h>
#include <RenderPassModule.h>
#include <DirectionalLight.h>
#include <SpotLight.h>
#include <PointLight.h>

#include <DescriptorBuffer.h>

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

class LightManager
{
private:
    const size_t MAX_NUM_LIGHT = 64;
    const uint32_t BIN_SLICES = 16;
    const uint32_t TILE_SIZE = 8;
    const uint32_t NUM_WORDS = (MAX_NUM_LIGHT + 31) / 32;
    const uint32_t MAX_NUM_TILES = 240 * 135;

    DeviceModule* deviceModule = nullptr;
    SwapChainModule* swapChainModule = nullptr;
    Camera* camera = nullptr;

    uint32_t currentNumLights = 0;
    std::unordered_map<std::string, std::shared_ptr<Light>> _lights;
    std::shared_ptr<LightManagerUniform> lightManagerUniform;
    std::vector<LightUniform> lightBuffer;
    std::vector<LightMap> sortedLight;
    std::vector<uint32_t> lights_bin;
    std::vector<uint32_t> lights_index;
    std::vector<uint32_t> light_tiles_bits;

    RenderPassModule* renderPassModule = nullptr;
    std::shared_ptr<ShaderModule> dir_shadow_map_shader = nullptr;
    std::shared_ptr<ShaderModule> omni_shadow_map_shader = nullptr;

public:
    static LightManager* instance;
    std::shared_ptr<UniformBufferObject>    lightUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    lightSSBO = nullptr;
    VkDeviceSize                            lightSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightIndexSSBO = nullptr;
    VkDeviceSize                            lightIndexSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightTilesSSBO = nullptr;
    VkDeviceSize                            lightTilesSSBOSize;
    std::shared_ptr<UniformBufferObject>    lightBinSSBO = nullptr;
    VkDeviceSize                            lightBinSSBOSize;

    std::vector<std::shared_ptr<DirectionalLight>> DirLights;
    std::vector<std::shared_ptr<SpotLight>> SpotLights;
    std::vector<std::shared_ptr<PointLight>> PointLights;

private:
    void AddLight(std::shared_ptr<Light> light_ptr, std::string& name);
    void SortingLights();
    void ComputeLightsLUT();
    void ComputeLightTiles();

public:
    static LightManager* getInstance();
    static void ResetInstance();
    LightManager();
    void AddDirShadowMapShader(std::shared_ptr<ShaderModule> shadow_mapping_shader);
    void AddOmniShadowMapShader(std::shared_ptr<ShaderModule> omni_shadow_mapping_shader);
    void CreateLight(LightType type, std::string name);
    std::shared_ptr<Light> GetLight(std::string name);
    void Update();
    void UpdateUniform();
    void UpdateUBOLight();
    void CleanLightUBO();
    void CleanLastResources();
    void SetCamera(Camera* camera_ptr);
};

#endif
