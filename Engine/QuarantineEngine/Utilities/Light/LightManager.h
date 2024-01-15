#pragma once
#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <unordered_map>
#include <memory>
#include <Light/Light.h>
#include <Camera.h>

struct LightMap
{
    uint32_t    id;
    float       projected_z;
    float       projected_z_min;
    float       projected_z_max;
};

struct BinElement
{
    uint32_t min;
    uint32_t max;
};

bool compareDistance(const LightMap& a, const LightMap& b);

class LightManager
{
private:
    const size_t MAX_NUM_LIGHT = 64;
    DeviceModule* deviceModule = nullptr;
    Camera* camera = nullptr;

    uint32_t currentNumLights = 0;
    std::unordered_map<std::string, std::shared_ptr<Light>> _lights;
    std::shared_ptr<LightManagerUniform> lightManagerUniform;
    std::vector<LightUniform> lightBuffer;
    std::vector<LightMap> sortedLight;
    std::vector<BinElement> binData;
    std::vector<uint32_t> lights_lut;
    std::vector<uint32_t> lights_index;

public:
    static LightManager* instance;
    std::shared_ptr<UniformBufferObject>    lightUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    lightSSBO = nullptr;
    VkDeviceSize                            lightSSBOSize;

private:
    void AddLight(std::shared_ptr<Light> light_ptr, std::string name);
    void SortingLights();
    void ComputeLightsLUT();

public:
    static LightManager* getInstance();
    static void ResetInstance();
    LightManager();
    void CreateLight(LightType type, std::string name);
    std::shared_ptr<Light> GetLight(std::string name);
    void UpdateUniform();
    void UpdateUBOLight();
    void CleanLightUBO();
    void CleanLastResources();
    void SetCamera(Camera* camera_ptr);
};

#endif
