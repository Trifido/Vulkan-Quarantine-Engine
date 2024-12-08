#pragma once

#ifndef CSM_MODULE_H
#define CSM_MODULE_H

#include <vulkan/vulkan.hpp>
#include <DeviceModule.h>
#include <SwapChainModule.h>

struct CascadeResource {
    VkFramebuffer frameBuffer;
    VkImageView view;
    float splitDepth;
    glm::mat4 viewProjMatrix;
    void destroy(VkDevice device)
    {
        vkDestroyImageView(device, view, nullptr);
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }
};

class CSMResources
{
private:
    DeviceModule* deviceModule;
    SwapChainModule* swapchainModule = nullptr;

    // CSM image
    VkImage CSMImage = VK_NULL_HANDLE;
    VkDeviceMemory CSMImageMemory = { VK_NULL_HANDLE };

public:
    static const int SHADOW_MAP_CASCADE_COUNT = 4;
    static QueueModule* queueModule;
    static VkCommandPool commandPool;
    uint32_t TextureSize;
    VkFormat shadowFormat;

    float DepthBiasConstant = 1.25f;
    float DepthBiasSlope = 1.75f;

public:
    std::shared_ptr<UniformBufferObject> shadowMapUBO = nullptr;
    std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT> cascadeResources;

    VkImageView CSMImageView = VK_NULL_HANDLE;
    VkSampler CSMSampler = VK_NULL_HANDLE;

private:
    void CreateCSMResources(std::shared_ptr<VkRenderPass> renderPass);
    void PrepareFramebuffer(std::shared_ptr<VkRenderPass> renderPass, int iCascade);

public:
    CSMResources();
    CSMResources(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUBOShadowMap(CSMUniform csmParameters);

    static VkSampler CreateCSMSampler(VkDevice device);
    static VkImageView CreateImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, int baseArrayLayer, int layerCount, uint32_t mipLevels = 1);
    static VkImage AllocateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory& mapMemory, uint32_t pixelSize,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);

    void Cleanup();
};

#endif // !CSM_MODULE_H
