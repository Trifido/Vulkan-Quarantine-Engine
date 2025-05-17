#pragma once

#ifndef SHADOW_MAPPING_MODULE_H
#define SHADOW_MAPPING_MODULE_H

#include <vulkan/vulkan.hpp>
#include <DeviceModule.h>
#include <SwapChainModule.h>


class OmniShadowResources
{
private:
    DeviceModule* deviceModule;
    SwapChainModule* swapchainModule = nullptr;

    // Shadow Render Image
    VkImage cubemapImage = VK_NULL_HANDLE;
    VkDeviceMemory cubemapMemory = { VK_NULL_HANDLE };

    // Offscreen shadow image
    VkImage framebufferDepthImage = VK_NULL_HANDLE;
    VkImageView framebufferDepthImageView = VK_NULL_HANDLE;
    VkDeviceMemory framebufferDepthImageMemory = { VK_NULL_HANDLE };

    std::array<VkImageView, 6> cubemapFacesImageViews = { VK_NULL_HANDLE };

public:
    static QueueModule* queueModule;
    static VkCommandPool commandPool;
    uint32_t TextureSize;
    VkFormat shadowFormat;

    float DepthBiasConstant = 1.25f;
    float DepthBiasSlope = 1.75f;

public:
    std::shared_ptr<UniformBufferObject> shadowMapUBO = nullptr;
    std::array<VkFramebuffer, 6> CubemapFacesFrameBuffers = { VK_NULL_HANDLE };
    VkImageView CubemapImageView = VK_NULL_HANDLE;
    VkSampler CubemapSampler = VK_NULL_HANDLE;

private:
    std::array<VkImageView, 6> CreateCubemapFacesImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

    VkImage CreateFramebufferDepthImage(VkDeviceMemory& deviceMemory);
    VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

    void CreateOmniShadowMapResources(std::shared_ptr<VkRenderPass> renderPass);
    void PrepareFramebuffers(std::shared_ptr<VkRenderPass> renderPass, VkImageView depthView, std::array<VkImageView, 6> cubemapView);


public:
    OmniShadowResources();
    OmniShadowResources(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUBOShadowMap(OmniShadowUniform omniParameters);

    static VkSampler CreateCubemapSampler(VkDevice device);
    static VkImageView CreateCubemapImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);
    static VkImage AllocateCubemapImage(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory& mapMemory, uint32_t pixelSize,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
    static void TransitionMultiImagesLayout(VkDevice device, VkImage& newImage, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);

    void Cleanup();
};

#endif // !SHADOW_MAPPING_MODULE_H
