#pragma once

#ifndef CUSTOM_TEXTURE_H
#define CUSTOM_TEXTURE_H

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include "TextureTypes.h"
#include "assimp/texture.h"
#include <vector>
#include <string>
#include <fstream>
#include <ktx.h>

enum class QEColorSpace : uint8_t
{
    Linear = 0, // UNORM
    SRGB = 1
};

struct KtxTranscodeSelection
{
    ktx_transcode_fmt_e ktxFormat;
    VkFormat vkFormat;
};

class CustomTexture : public TextureManagerModule
{
private:
    int                 texWidth, texHeight, texChannels;

public:
    std::vector<std::string>    texturePaths;
    VkSampler                   textureSampler;
    static VkCommandPool        commandPool;
    TEXTURE_TYPE                type;

private:
    void RecordTransitionImageLayout(
        VkCommandBuffer cmd,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageSubresourceRange range);

    void RecordCopyBufferToImage(
        VkCommandBuffer cmd,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height);

    void RecordCopyBufferToCubemapImage(
        VkCommandBuffer cmd,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels);

    void RecordCopyBufferImagesToCubemapImage(
        VkCommandBuffer cmd,
        VkBuffer buffer,
        VkImage image,
        uint32_t textureWidth,
        VkDeviceSize faceTextureSize,
        uint32_t mipLevels);

    void RecordGenerateMipmaps(
        VkCommandBuffer cmd,
        VkImage image,
        VkFormat imageFormat,
        int32_t texWidth,
        int32_t texHeight,
        uint32_t mipLevels);

    int GetChannelCount(VkFormat format);

    KtxTranscodeSelection ChooseKtxTranscodeFormat(QEColorSpace cs);
    void createTextureFromKtx2(const std::string& path, QEColorSpace cs);

public:
    CustomTexture();
    CustomTexture(std::string path, TEXTURE_TYPE type);
    CustomTexture(std::vector<std::string> path);
    CustomTexture(aiTexel* data, unsigned int width, unsigned int height, TEXTURE_TYPE type);
    CustomTexture(unsigned int width, unsigned int height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, TEXTURE_TYPE type);
    CustomTexture(std::string path, TEXTURE_TYPE type, QEColorSpace cs);
    void createTextureImage(std::string path = NULL);
    void createTextureImage(std::string path, QEColorSpace cs);
    void createCubemapTextureImage(std::string path = NULL);
    void createCubemapTextureImage(std::vector<std::string> paths);
    void createTextureRawImage(aiTexel* rawData, unsigned int width, unsigned int height);
    void createTextureImageView(VkFormat imageFormat);
    void createTextureSampler();
    void createCubemapTextureSampler();
    void cleanup();
    void SaveTexturePath(std::ofstream& file);
};

#endif
