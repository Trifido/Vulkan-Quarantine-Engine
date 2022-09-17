#pragma once

#ifndef TEXTURE_H
#define TEXTURE_H
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include "TextureTypes.h"

class Texture : public TextureManagerModule
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
    Texture();
    Texture(std::string path, TEXTURE_TYPE type);
    void createTextureImage(std::string path);
    void createTextureImageView();
    void createTextureSampler();
    void cleanup();
};

#endif
