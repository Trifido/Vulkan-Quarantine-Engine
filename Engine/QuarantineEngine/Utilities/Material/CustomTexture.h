#pragma once

#ifndef CUSTOM_TEXTURE_H
#define CUSTOM_TEXTURE_H
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include "TextureTypes.h"
#include "assimp/texture.h"
#include <vector>

using namespace std;

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
    void copyBufferImagesToCubemapImage(VkBuffer buffer, VkImage image, uint32_t textureWidth, VkDeviceSize faceTextureSize, uint32_t mipLevels);
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
public:
    CustomTexture();
    CustomTexture(string path, TEXTURE_TYPE type);
    CustomTexture(vector<string> path);
    CustomTexture(aiTexel* data, unsigned int width, unsigned int height, TEXTURE_TYPE type);
    void createTextureImage(string path = NULL);
    void createCubemapTextureImage(string path = NULL);
    void createCubemapTextureImage(vector<string> paths);
    void createTextureRawImage(aiTexel* rawData, unsigned int width, unsigned int height);
    void createTextureImageView(VkFormat imageFormat);
    void createTextureSampler();
    void createCubemapTextureSampler();
    void cleanup();
};

#endif
