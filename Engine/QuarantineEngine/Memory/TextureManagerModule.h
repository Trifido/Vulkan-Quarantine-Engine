#pragma once

#ifndef TEXTURE_MANAGER_MODULE_H
#define TEXTURE_MANAGER_MODULE_H

#include "ImageMemoryTools.h"
#include "QueueModule.h"
#include "DeviceModule.h"
#include <SwapChainModule.h>

class TextureManagerModule
{
public:
    uint32_t        mipLevels = 1;
    DeviceModule*   deviceModule;
    VkImageView     imageView;
    static QueueModule* queueModule;
protected:
    VkImage         image;
    VkDeviceMemory  deviceMemory;
    VkCommandPool*  ptrCommandPool;
    SwapChainModule* swapchainModule = nullptr;

public:
    TextureManagerModule();
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlag);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
    void createCubeMapImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
    virtual void cleanup();
};

#endif



