#include "QEProjectIconCache.h"

#include <cstring>
#include <stdexcept>

#include <imgui_impl_vulkan.h>
#include <stb_image.h>

#include <CommandPoolModule.h>
#include <DeviceModule.h>
#include <ImageMemoryTools.h>
#include <QueueModule.h>
#include <SyncTool.h>
#include <Helpers/QEMemoryTrack.h>

QEProjectIconCache::~QEProjectIconCache()
{
    Cleanup();
}

bool QEProjectIconCache::Initialize()
{
    Cleanup();

    bool ok = true;
    ok &= LoadIconTexture(QEAssetType::Folder, "../../src/QuarantineEditor/Icons/folder.png");
    ok &= LoadIconTexture(QEAssetType::Scene, "../../src/QuarantineEditor/Icons/scene.png");
    ok &= LoadIconTexture(QEAssetType::Material, "../../src/QuarantineEditor/Icons/material.png");
    ok &= LoadIconTexture(QEAssetType::Shader, "../../src/QuarantineEditor/Icons/shader.png");
    ok &= LoadIconTexture(QEAssetType::Graph, "../../src/QuarantineEditor/Icons/graph.png");
    ok &= LoadIconTexture(QEAssetType::Spv, "../../src/QuarantineEditor/Icons/spv.png");
    ok &= LoadIconTexture(QEAssetType::Texture, "../../src/QuarantineEditor/Icons/texture.png");
    ok &= LoadIconTexture(QEAssetType::Mesh, "../../src/QuarantineEditor/Icons/mesh.png");
    ok &= LoadIconTexture(QEAssetType::Animation, "../../src/QuarantineEditor/Icons/animation.png");
    ok &= LoadIconTexture(QEAssetType::NavigateUp, "../../src/QuarantineEditor/Icons/up.png");
    return ok;
}

void QEProjectIconCache::Cleanup()
{
    auto device = DeviceModule::getInstance();

    for (auto& [type, icon] : _iconTextures)
    {
        if (icon.DescriptorSet != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(icon.DescriptorSet);
            icon.DescriptorSet = VK_NULL_HANDLE;
            icon.ImGuiTexture = 0;
        }

        if (icon.Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device->device, icon.Sampler, nullptr);
            icon.Sampler = VK_NULL_HANDLE;
        }

        if (icon.ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device->device, icon.ImageView, nullptr);
            icon.ImageView = VK_NULL_HANDLE;
        }

        if (icon.Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device->device, icon.Image, nullptr);
            icon.Image = VK_NULL_HANDLE;
        }

        if (icon.Memory != VK_NULL_HANDLE)
        {
            QE_FREE_MEMORY(device->device, icon.Memory, "QEProjectIconCache::Cleanup");
            icon.Memory = VK_NULL_HANDLE;
        }
    }

    _iconTextures.clear();
}

const QEIconTexture* QEProjectIconCache::GetIcon(QEAssetType type) const
{
    auto it = _iconTextures.find(type);
    if (it == _iconTextures.end())
        return nullptr;

    return it->second.IsValid ? &it->second : nullptr;
}

bool QEProjectIconCache::LoadIconTexture(QEAssetType type, const std::filesystem::path& filePath)
{
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;

    stbi_uc* pixels = stbi_load(filePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels || texWidth <= 0 || texHeight <= 0)
        return false;

    const VkDeviceSize imageSize =
        static_cast<VkDeviceSize>(texWidth) *
        static_cast<VkDeviceSize>(texHeight) * 4;

    auto device = DeviceModule::getInstance();

    QEIconTexture icon{};
    icon.Width = static_cast<uint32_t>(texWidth);
    icon.Height = static_cast<uint32_t>(texHeight);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    if (!CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory))
    {
        stbi_image_free(pixels);
        return false;
    }

    void* mappedData = nullptr;
    if (vkMapMemory(device->device, stagingBufferMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS)
    {
        QE_DESTROY_BUFFER(device->device, stagingBuffer, "QEProjectIconCache::LoadIconTexture");
        QE_FREE_MEMORY(device->device, stagingBufferMemory, "QEProjectIconCache::LoadIconTexture");
        stbi_image_free(pixels);
        return false;
    }

    std::memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device->device, stagingBufferMemory);
    stbi_image_free(pixels);

    if (!CreateImage(
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        icon.Image,
        icon.Memory))
    {
        QE_DESTROY_BUFFER(device->device, stagingBuffer, "QEProjectIconCache::LoadIconTexture");
        QE_FREE_MEMORY(device->device, stagingBufferMemory, "QEProjectIconCache::LoadIconTexture");
        return false;
    }

    TransitionImageLayout(
        icon.Image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(
        stagingBuffer,
        icon.Image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight));

    TransitionImageLayout(
        icon.Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    QE_DESTROY_BUFFER(device->device, stagingBuffer, "QEProjectIconCache::LoadIconTexture");
    QE_FREE_MEMORY(device->device, stagingBufferMemory, "QEProjectIconCache::LoadIconTexture");

    icon.ImageView = CreateImageView(
        icon.Image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT);

    if (icon.ImageView == VK_NULL_HANDLE)
    {
        vkDestroyImage(device->device, icon.Image, nullptr);
        QE_FREE_MEMORY(device->device, icon.Memory, "QEProjectIconCache::LoadIconTexture");
        return false;
    }

    icon.Sampler = CreateSampler();
    if (icon.Sampler == VK_NULL_HANDLE)
    {
        vkDestroyImageView(device->device, icon.ImageView, nullptr);
        vkDestroyImage(device->device, icon.Image, nullptr);
        QE_FREE_MEMORY(device->device, icon.Memory, "QEProjectIconCache::LoadIconTexture");
        return false;
    }

    icon.DescriptorSet = ImGui_ImplVulkan_AddTexture(
        icon.Sampler,
        icon.ImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    icon.ImGuiTexture = (ImTextureID)icon.DescriptorSet;
    icon.IsValid = (icon.DescriptorSet != VK_NULL_HANDLE);

    if (!icon.IsValid)
    {
        vkDestroySampler(device->device, icon.Sampler, nullptr);
        vkDestroyImageView(device->device, icon.ImageView, nullptr);
        vkDestroyImage(device->device, icon.Image, nullptr);
        QE_FREE_MEMORY(device->device, icon.Memory, "QEProjectIconCache::LoadIconTexture");
        return false;
    }

    _iconTextures[type] = icon;
    return true;
}

uint32_t QEProjectIconCache::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    auto device = DeviceModule::getInstance();
    return IMT::findMemoryType(typeFilter, properties, device->physicalDevice);
}

bool QEProjectIconCache::CreateBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    auto device = DeviceModule::getInstance();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        QE_DESTROY_BUFFER(device->device, buffer, "QEProjectIconCache::CreateBuffer");
        buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(device->device, buffer, bufferMemory, 0);
    return true;
}

bool QEProjectIconCache::CreateImage(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory)
{
    auto device = DeviceModule::getInstance();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        vkDestroyImage(device->device, image, nullptr);
        image = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(device->device, image, imageMemory, 0);
    return true;
}

void QEProjectIconCache::TransitionImageLayout(
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    auto device = DeviceModule::getInstance();
    auto commandPool = CommandPoolModule::getInstance();
    auto queueModule = QueueModule::getInstance();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        device->device,
        commandPool->getCommandPool());

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        endSingleTimeCommands(
            device->device,
            queueModule->graphicsQueue,
            commandPool->getCommandPool(),
            commandBuffer);

        throw std::runtime_error("Unsupported image layout transition in QEProjectIconCache.");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(
        device->device,
        queueModule->graphicsQueue,
        commandPool->getCommandPool(),
        commandBuffer);
}

void QEProjectIconCache::CopyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height)
{
    auto device = DeviceModule::getInstance();
    auto commandPool = CommandPoolModule::getInstance();
    auto queueModule = QueueModule::getInstance();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        device->device,
        commandPool->getCommandPool());

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    endSingleTimeCommands(
        device->device,
        queueModule->graphicsQueue,
        commandPool->getCommandPool(),
        commandBuffer);
}

VkImageView QEProjectIconCache::CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    auto device = DeviceModule::getInstance();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    return imageView;
}

VkSampler QEProjectIconCache::CreateSampler()
{
    auto device = DeviceModule::getInstance();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.mipLodBias = 0.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    return sampler;
}
