#pragma once

#ifndef TEXTURE_MODULE_H
#define TEXTURE_MODULE_H
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.h>

#include "CommandPoolModule.h"
#include "BufferManageModule.h"
#include "TextureManagerModule.h"

class CommandPoolModule;
class BufferManageModule;

class TextureModule : public TextureManagerModule
{
private:
    int                 texWidth, texHeight, texChannels;
    BufferManageModule* bManagerModule;
public:
    VkSampler           textureSampler;

private:
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
public:
    TextureModule();
    void createTextureImage(BufferManageModule& managerModule, QueueModule& queueModule, CommandPoolModule& commandPoolModule);
    void createTextureImageView();
    void createTextureSampler();
    void cleanup();
};

#endif
