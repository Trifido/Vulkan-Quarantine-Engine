#pragma once

#ifndef CSM_DESCRIPTOR
#define CSM_DESCRIPTOR

#include <vulkan/vulkan.h>
#include <DeviceModule.h>
#include <ShaderModule.h>
#include <CSMResources.h>

constexpr uint32_t                  NUM_CSM_SETS = 2;
constexpr uint32_t                  NUM_CSM_PASSES = 2;
constexpr uint32_t                  MAX_NUM_DIR_LIGHTS = 10;

class CSMDescriptorsManager
{
private:
    DeviceModule*                   deviceModule = nullptr;
    VkPipelineLayout                pipelineLayout[NUM_CSM_PASSES];

    VkDescriptorSetLayout           renderDescriptorSetLayout;
    VkDescriptorSetLayout           offscreenDescriptorSetLayout;

    VkDescriptorPool                renderDescriptorPool;
    VkDescriptorPool                offscreenDescriptorPool;

    // Offscreen resources
    uint32_t    _numDirLights = 0;
    std::vector<std::shared_ptr<UniformBufferObject>> csmOffscreenUBOs;
    std::vector<VkDescriptorBufferInfo> offscreenBuffersInfo;

    // Render resources
    std::vector<VkDescriptorBufferInfo> renderBuffersInfo;
    std::vector<VkDescriptorImageInfo> renderDescriptorImageInfo;
    UniformBufferObject csmRenderSplitBuffer;
    UniformBufferObject csmRenderViewProjBuffer;
    VkDeviceSize csmSplitDataBufferSize;
    VkDeviceSize csmViewProjDataBufferSize;

    //Bound resources
    std::vector<std::shared_ptr<std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT>> > csmResources;
    std::vector<float> csmSplitDataResources;
    std::vector<glm::mat4> csmViewProjDataResources;

    // ImageViews & Samplers
    std::vector<VkImageView>    _imageViews;
    std::vector<VkSampler>      _samplers;

    VkDeviceMemory placeholderMemory = VK_NULL_HANDLE;
    VkImage placeholderImage = VK_NULL_HANDLE;
    VkImageView placeholderImageView = VK_NULL_HANDLE;
    VkSampler placeholderSampler = VK_NULL_HANDLE;

public:
    VkDescriptorSet offscreenDescriptorSets[NUM_CSM_SETS][MAX_NUM_DIR_LIGHTS];
    VkDescriptorSet renderDescriptorSets[NUM_CSM_SETS];

public:
    CSMDescriptorsManager();
    void AddDirLightResources(std::shared_ptr<UniformBufferObject> offscreenShadowMapUBO, VkImageView imageView, VkSampler sampler);
    void DeleteDirLightResources(int idPos);
    void BindResources(std::shared_ptr<std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT>> resources);
    void UpdateResources(int currentFrame);
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
    void SetCSMDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType, uint32_t binding);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void CreateCSMPlaceHolder();
};

#endif // !CSM_DESCRIPTOR

