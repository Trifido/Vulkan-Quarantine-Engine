#pragma once

#ifndef CUSTOM_TEXTURE_H
#define CUSTOM_TEXTURE_H
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include "TextureTypes.h"
#include "assimp/texture.h"

class CustomTexture : public TextureManagerModule
{
private:
    int                 texWidth, texHeight, texChannels;
public:
    VkSampler           textureSampler;
    static VkCommandPool commandPool;
    TEXTURE_TYPE        type;

private:
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
    void copyBufferToCubemapImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    CustomTexture();
    CustomTexture(std::string path, TEXTURE_TYPE type);
    CustomTexture(aiTexel* data, unsigned int width, unsigned int height, TEXTURE_TYPE type);
    void createTextureImage(std::string path = NULL);
    void createCubemapTextureImage(std::string path = NULL);
    void createTextureRawImage(aiTexel* rawData, unsigned int width, unsigned int height);
    void createTextureImageView();
    void createTextureSampler();
    void createCubemapTextureSampler();
    void cleanup();
};

#endif
