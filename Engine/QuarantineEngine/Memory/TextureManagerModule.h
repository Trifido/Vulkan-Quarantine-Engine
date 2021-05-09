#pragma once

#ifndef TEXTURE_MANAGER_MODULE_H
#define TEXTURE_MANAGER_MODULE_H

#include "ImageMemoryTools.h"
#include "QueueModule.h"
#include "DeviceModule.h"

class TextureManagerModule
{
public:
    DeviceModule*   deviceModule;
    VkImageView     imageView;
protected:
    VkImage         image;
    VkDeviceMemory  deviceMemory;
    VkQueue*        ptrGraphicsQueue;
    VkCommandPool*  ptrCommandPool;

protected:
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
public:
    TextureManagerModule();
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
};

#endif



