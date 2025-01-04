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
private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* deviceModule;
    Camera* camera = nullptr;

    shared_ptr<ShaderModule> skybox_cubemap_shader;
    shared_ptr<GeometryComponent> _Mesh = nullptr;

    // Descriptor set
    vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;

    // Cubemap resources
    shared_ptr<CustomTexture> skyboxTexture = nullptr;

    VkDescriptorBufferInfo buffersInfo;
    VkDescriptorImageInfo imageInfo;

private:
    void CreateDescriptorPool();
    void CreateDescriptorSet();
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);

    void SetCubeMapDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding);

    string GetAbsolutePath(string relativePath);
public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    void AddSkyboxResources(string texturePath);
    void AddSkyboxResources(vector<string> texturePaths);
    void InitializeResources();
    void SetCamera(Camera* cameraPtr);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t frameIdx);
    void Cleanup();
    void CleanLastResources();
};

#endif // !ATMOSPHERE_SYSTEM_H


