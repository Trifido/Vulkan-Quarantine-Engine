#pragma once

#ifndef CUSTOM_TEXTURE_H
#define CUSTOM_TEXTURE_H
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include "TextureTypes.h"

class CustomTexture : public TextureManagerModule
{
private:
    int                 texWidth, texHeight, texChannels;
public:
    VkSampler           textureSampler;
    static VkCommandPool commandPool;
    TEXTURE_TYPE        type;

private:
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    CustomTexture();
    CustomTexture(std::string path, TEXTURE_TYPE type);
    void createTextureImage(std::string path = NULL);
    void createTextureImageView();
    void createTextureSampler();
    void cleanup();
};

#endif
