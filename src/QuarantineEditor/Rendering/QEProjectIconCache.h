#pragma once
#ifndef QE_PROJECT_ICON_CACHE
#define QE_PROJECT_ICON_CACHE

#include <filesystem>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <Browser/QEProjectAssetItem.h>
#include <imgui.h>

struct QEIconTexture
{
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkSampler Sampler = VK_NULL_HANDLE;

    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
    ImTextureID ImGuiTexture = 0;

    uint32_t Width = 0;
    uint32_t Height = 0;
    bool IsValid = false;
};

class QEProjectIconCache
{
public:
    ~QEProjectIconCache();

    bool Initialize();
    void Cleanup();

    const QEIconTexture* GetIcon(QEAssetType type) const;

private:
    bool LoadIconTexture(QEAssetType type, const std::filesystem::path& filePath);

    bool CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);

    bool CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory);

    void TransitionImageLayout(
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

    void CopyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height);

    VkImageView CreateImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags);

    VkSampler CreateSampler();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

private:
    std::unordered_map<QEAssetType, QEIconTexture> _iconTextures;
};

#endif
