#pragma once

#ifndef CSM_MODULE_H
#define CSM_MODULE_H

#include <vulkan/vulkan.hpp>
#include <DeviceModule.h>
#include <SwapChainModule.h>

constexpr uint32_t  SHADOW_MAP_CASCADE_COUNT = 4;

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
    static QueueModule* queueModule;
    static VkCommandPool commandPool;
    uint32_t TextureSize;
    VkFormat shadowFormat;

    float DepthBiasConstant = 1.25f;
    float DepthBiasSlope = 1.75f;

public:
    std::shared_ptr<UniformBufferObject> OffscreenShadowMapUBO = nullptr;
    std::shared_ptr<std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT>> CascadeResourcesPtr;

    VkImageView CSMImageView = VK_NULL_HANDLE;
    VkSampler CSMSampler = VK_NULL_HANDLE;

private:
    void CreateCSMResources(std::shared_ptr<VkRenderPass> renderPass);
    void PrepareFramebuffer(std::shared_ptr<VkRenderPass> renderPass, int iCascade);

public:
    CSMResources();
    CSMResources(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateOffscreenUBOShadowMap();
    void TransitionImageLayout(VkDevice device, VkImage& newImage, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);

    static VkSampler CreateCSMSampler(VkDevice device);
    static VkImageView CreateImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, int baseArrayLayer, int layerCount, uint32_t mipLevels = 1);
    static VkImage AllocateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory& mapMemory, uint32_t pixelSize,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);

    void Cleanup();
};

#endif // !CSM_MODULE_H
