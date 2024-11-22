#pragma once

#ifndef POINT_SHADOW_DESCRIPTOR
#define POINT_SHADOW_DESCRIPTOR

#include <vulkan/vulkan.h>
#include <DeviceModule.h>
#include <ShaderModule.h>

constexpr uint32_t                  NUM_POINT_SHADOW_SETS = 2;
constexpr uint32_t                  NUM_POINT_SHADOW_PASSES = 2;
constexpr uint32_t                  MAX_NUM_POINT_LIGHTS = 10;

class PointShadowDescriptorsManager
{
private:
    DeviceModule*                   deviceModule = nullptr;
    VkPipelineLayout                pipelineLayout[NUM_POINT_SHADOW_PASSES];

    VkDescriptorSetLayout           renderDescriptorSetLayout;
    VkDescriptorSetLayout           offscreenDescriptorSetLayout;

    VkDescriptorPool                renderDescriptorPool;
    VkDescriptorPool                offscreenDescriptorPool;

    // Offscreen resources
    uint32_t    _numPointLights = 0;
    std::vector<std::shared_ptr<UniformBufferObject>> shadowMapUBOs;
    std::vector<VkDescriptorBufferInfo> offscreenBuffersInfo;

    // Render resources
    std::vector<VkDescriptorBufferInfo> renderBuffersInfo;
    std::vector<VkDescriptorImageInfo> shadowPointsImageInfo;
    std::shared_ptr<UniformBufferObject> pointlightIdBuffer;
    VkDeviceSize sizePointlightIdBuffer;

    // ImageViews & Samplers
    std::vector<VkImageView>    _imageViews;
    std::vector<VkSampler>      _samplers;

    VkDeviceMemory placeholderMemory = VK_NULL_HANDLE;
    VkImage placeholderImage = VK_NULL_HANDLE;
    VkImageView placeholderImageView = VK_NULL_HANDLE;
    VkSampler placeholderSampler = VK_NULL_HANDLE;

public:
    VkDescriptorSet offscreenDescriptorSets[NUM_POINT_SHADOW_SETS][MAX_NUM_POINT_LIGHTS];
    VkDescriptorSet renderDescriptorSets[NUM_POINT_SHADOW_SETS];

public:
    PointShadowDescriptorsManager();
    void AddPointLightResources(std::shared_ptr<UniformBufferObject> shadowMapUBO, VkImageView imageView, VkSampler sampler);
    void InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr);
    void Clean();

private:
    void CreateOffscreenDescriptorPool();
    void CreateOffscreenDescriptorSet();
    void SetOffscreenDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);
    void CreateRenderDescriptorPool();
    void CreateRenderDescriptorSet();
    VkDescriptorSetLayout CreateRenderDescriptorSetLayout();
    void SetRenderDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize);
    void SetCubeMapDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void CreateCubemapPlaceHolder();
};

#endif // !POINT_SHADOW_DESCRIPTOR

