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
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    TextureModule();
    void createTextureImage(std::string path, BufferManageModule& managerModule, QueueModule& queueModule, CommandPoolModule& commandPoolModule);
    void createTextureImageView();
    void createTextureSampler();
    void cleanup();
};

#endif
