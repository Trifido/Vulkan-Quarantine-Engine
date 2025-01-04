#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <ShaderModule.h>
#include <CustomTexture.h>
#include <Camera.h>
#include <QESingleton.h>

using namespace std;

class AtmosphereSystem : public QESingleton<AtmosphereSystem>
{
public:
    enum ENVIRONMENT_TYPE
    {
        NONEMAP = -1,
        CUBEMAP = 0,
        SPHERICALMAP = 1,
    };

private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* deviceModule;
    Camera* camera = nullptr;

    // Environment type
    ENVIRONMENT_TYPE environmentType;

    // Skybox shader
    const vector<string> shaderPaths = {
        "Atmosphere/skybox_cubemap_vert.spv",
        "Atmosphere/sky_spherical_map_vert.spv",
        "Atmosphere/skybox_cubemap_frag.spv",
        "Atmosphere/sky_spherical_map_frag.spv",
    };
    shared_ptr<ShaderModule> environment_shader;

    // Mesh
    shared_ptr<GeometryComponent> _Mesh = nullptr;

    // Descriptor set
    vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    // Cubemap resources
    shared_ptr<CustomTexture> environmentTexture = nullptr;

    VkDescriptorBufferInfo buffersInfo;
    VkDescriptorImageInfo imageInfo;

private:
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);

    void SetSamplerDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding);

    string GetAbsolutePath(string relativePath, string filename);

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    void AddTextureResources(string texturePath);
    void AddTextureResources(vector<string> texturePaths);
    void SetUpResources(ENVIRONMENT_TYPE type);
    void SetCamera(Camera* cameraPtr);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx);
    void Cleanup();
    void CleanLastResources();
};

#endif // !ATMOSPHERE_SYSTEM_H


