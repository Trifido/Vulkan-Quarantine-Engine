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
    VkImageView     imageView = { VK_NULL_HANDLE };
    VkImage         image = { VK_NULL_HANDLE };
    VkImageLayout   currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    static QueueModule* queueModule;
protected:
    VkDeviceMemory  deviceMemory = { VK_NULL_HANDLE };
    VkCommandPool*  ptrCommandPool = { VK_NULL_HANDLE };
    SwapChainModule* swapchainModule = nullptr;

public:
    TextureManagerModule();
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels = 1, uint32_t arrayLayers = 1, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
    virtual void cleanup();
};

#endif



