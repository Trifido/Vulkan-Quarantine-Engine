#pragma once

#ifndef POINT_SHADOW_DESCRIPTOR
#define POINT_SHADOW_DESCRIPTOR

#include <vulkan/vulkan.h>
#include <DeviceModule.h>
#include <ShaderModule.h>

constexpr uint32_t NUM_POINT_SHADOW_SETS = 2;
constexpr uint32_t NUM_POINT_SHADOW_PASSES = 2;
constexpr uint32_t MAX_NUM_POINT_LIGHTS = 10;

class PointShadowDescriptorsManager
{
private:
    DeviceModule* deviceModule = nullptr;
    VkPipelineLayout pipelineLayout[NUM_POINT_SHADOW_PASSES]{};

    VkDescriptorSetLayout renderDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout offscreenDescriptorSetLayout = VK_NULL_HANDLE;

    VkDescriptorPool renderDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool offscreenDescriptorPool = VK_NULL_HANDLE;

    uint32_t _numPointLights = 0;
    std::vector<std::shared_ptr<UniformBufferObject>> shadowMapUBOs;

    std::vector<VkDescriptorImageInfo> renderDescriptorImageInfo;

    std::vector<VkImageView> _imageViews;
    std::vector<VkSampler> _samplers;

    VkDeviceMemory placeholderMemory = VK_NULL_HANDLE;
    VkImage placeholderImage = VK_NULL_HANDLE;
    VkImageView placeholderImageView = VK_NULL_HANDLE;
    VkSampler placeholderSampler = VK_NULL_HANDLE;

    VkDescriptorBufferInfo offscreenBufferInfo{};

public:
    VkDescriptorSet offscreenDescriptorSets[NUM_POINT_SHADOW_SETS][MAX_NUM_POINT_LIGHTS]{};
    VkDescriptorSet renderDescriptorSets[NUM_POINT_SHADOW_SETS]{};

public:
    PointShadowDescriptorsManager();

    void AddPointLightResources(std::shared_ptr<UniformBufferObject> shadowMapUBO, VkImageView imageView, VkSampler sampler);
    void DeletePointLightResources(int idPos);
    void InitializeDescriptorSetLayouts(std::shared_ptr<ShaderModule> offscreen_shader_ptr);
    void ResetSceneState();
    void Clean();

private:
    void CreateOffscreenDescriptorPool();
    void CreateRenderDescriptorPool();

    void CreateOffscreenDescriptorSet();
    void CreateRenderDescriptorSet();

    void AllocateOffscreenDescriptorSetForLight(uint32_t lightIndex);
    void UpdateRenderDescriptorSets();

    void SetOffscreenDescriptorWrite(
        VkWriteDescriptorSet& descriptorWrite,
        VkDescriptorSet descriptorSet,
        VkDescriptorType descriptorType,
        uint32_t binding,
        VkBuffer buffer,
        VkDeviceSize bufferSize);

    void SetCubeMapDescriptorWrite(
        VkWriteDescriptorSet& descriptorWrite,
        VkDescriptorSet descriptorSet,
        VkDescriptorType descriptorType,
        uint32_t binding);

    VkDescriptorSetLayout CreateRenderDescriptorSetLayout();
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void CreateCubemapPlaceHolder();

    void WaitForGpuIdle() const;
};



namespace QE
{
    using ::PointShadowDescriptorsManager;
} // namespace QE
// QE namespace aliases
#endif
