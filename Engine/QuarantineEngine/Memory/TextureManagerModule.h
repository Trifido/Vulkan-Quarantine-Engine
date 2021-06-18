#pragma once

#ifndef TEXTURE_MANAGER_MODULE_H
#define TEXTURE_MANAGER_MODULE_H

#include "ImageMemoryTools.h"
#include "QueueModule.h"
#include "DeviceModule.h"

class TextureManagerModule
{
public:
    uint32_t        mipLevels = 1;
    DeviceModule*   deviceModule;
    QueueModule*    queueModule;
    VkImageView     imageView;
protected:
    VkImage         image;
    VkDeviceMemory  deviceMemory;
    VkCommandPool*  ptrCommandPool;

public:
    TextureManagerModule();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlag);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels=1, VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT);
    virtual void cleanup();
};

#endif



