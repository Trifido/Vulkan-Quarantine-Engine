#pragma once

#ifndef SPOT_SHADOW_DESCRIPTOR_H
#define SPOT_SHADOW_DESCRIPTOR_H

#include <vulkan/vulkan.h>
#include <DeviceModule.h>
#include <ShaderModule.h>
#include <SpotShadowResources.h>

constexpr uint32_t NUM_SPOT_SHADOW_SETS = 2;
constexpr uint32_t MAX_NUM_SPOT_LIGHTS = 10;

class SpotShadowDescriptorsManager
{
private:
    DeviceModule* deviceModule = nullptr;

    VkDescriptorSetLayout renderDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout offscreenDescriptorSetLayout = VK_NULL_HANDLE;

    VkDescriptorPool renderDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool offscreenDescriptorPool = VK_NULL_HANDLE;

    uint32_t _numSpotLights = 0;
    std::vector<std::shared_ptr<UniformBufferObject>> offscreenShadowMapUBOs;
    std::vector<std::shared_ptr<SpotShadowResources>> shadowResources;
    std::vector<VkImageView> _imageViews;
    std::vector<VkSampler> _samplers;

    std::vector<VkDescriptorImageInfo> renderDescriptorImageInfo;
    std::vector<VkDescriptorBufferInfo> renderBuffersInfo;

    UniformBufferObject spotRenderViewProjBuffer;
    VkDeviceSize spotViewProjDataBufferSize = 0;
    std::vector<glm::mat4> spotViewProjDataResources;

    VkDeviceMemory placeholderMemory = VK_NULL_HANDLE;
    VkImage placeholderImage = VK_NULL_HANDLE;
    VkImageView placeholderImageView = VK_NULL_HANDLE;
    VkSampler placeholderSampler = VK_NULL_HANDLE;

    VkDescriptorBufferInfo offscreenBufferInfo{};

public:
    VkDescriptorSet offscreenDescriptorSets[NUM_SPOT_SHADOW_SETS][MAX_NUM_SPOT_LIGHTS]{};
    VkDescriptorSet renderDescriptorSets[NUM_SPOT_SHADOW_SETS]{};

public:
    SpotShadowDescriptorsManager();

    void AddSpotLightResources(
        std::shared_ptr<UniformBufferObject> offscreenShadowMapUBO,
        VkImageView imageView,
        VkSampler sampler);
    void DeleteSpotLightResources(int idPos);
    void BindResources(const std::shared_ptr<SpotShadowResources>& resources);
    void UpdateResources(int currentFrame);
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

    void SetSpotDescriptorWrite(
        VkWriteDescriptorSet& descriptorWrite,
        VkDescriptorSet descriptorSet,
        VkDescriptorType descriptorType,
        uint32_t binding);

    void SetRenderDescriptorWrite(
        VkWriteDescriptorSet& descriptorWrite,
        VkDescriptorSet descriptorSet,
        VkDescriptorType descriptorType,
        uint32_t binding,
        VkBuffer buffer,
        VkDeviceSize bufferSize);

    VkDescriptorSetLayout CreateRenderDescriptorSetLayout();
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void CreateShadowPlaceholder();
    void WaitForGpuIdle() const;
};



namespace QE
{
    using ::SpotShadowDescriptorsManager;
} // namespace QE
// QE namespace aliases
#endif
