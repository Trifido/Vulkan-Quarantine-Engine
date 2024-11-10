#include "OmniShadowResources.h"
#include "ImageMemoryTools.h"
#include "FrameBufferModule.h"
#include <SynchronizationModule.h>
#include <SyncTool.h>

QueueModule* OmniShadowResources::queueModule;
VkCommandPool OmniShadowResources::commandPool;

OmniShadowResources::OmniShadowResources()
{
    this->TextureSize = 1024;
    this->shadowFormat = VK_FORMAT_D32_SFLOAT;
    this->deviceModule = DeviceModule::getInstance();
    this->swapchainModule = SwapChainModule::getInstance();

    this->shadowMapUBO = std::make_shared<UniformBufferObject>();
    this->shadowMapUBO->CreateUniformBuffer(sizeof(OmniShadowUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

OmniShadowResources::OmniShadowResources(std::shared_ptr<VkRenderPass> renderPass) : OmniShadowResources()
{
    this->shadowFormat = VK_FORMAT_R32_SFLOAT;
    this->CreateOmniShadowMapResources(renderPass);
}

VkImage OmniShadowResources::CreateFramebufferDepthImage(VkDeviceMemory& deviceImageMemory)
{
    VkImage resultImage = {};
    // Color attachment
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = this->shadowFormat;
    imageCreateInfo.extent.width = this->TextureSize;
    imageCreateInfo.extent.height = this->TextureSize;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // Image of the framebuffer is blit source
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Depth stencil attachment
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    vkCreateImage(this->deviceModule->device, &imageCreateInfo, nullptr, &resultImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(this->deviceModule->device, resultImage, &memReqs);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = IMT::findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceModule->physicalDevice);
    vkAllocateMemory(this->deviceModule->device, &memAlloc, nullptr, &deviceImageMemory);
    vkBindImageMemory(this->deviceModule->device, resultImage, deviceImageMemory, 0);

    return resultImage;
}

VkImageView OmniShadowResources::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

VkImage OmniShadowResources::AllocateCubemapImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples)
{
    VkImage result;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(deviceModule->device, &imageInfo, nullptr, &result) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cube map image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(deviceModule->device, result, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = IMT::findMemoryType(memRequirements.memoryTypeBits, properties, deviceModule->physicalDevice);

    if (vkAllocateMemory(deviceModule->device, &allocInfo, nullptr, &cubemapMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate cube map image memory!");
    }

    vkBindImageMemory(deviceModule->device, result, cubemapMemory, 0);

    return result;
}

VkImageView OmniShadowResources::CreateCubemapImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageView resultImageView = {};

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R };

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    //VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &resultImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cubemap texture image view!");
    }

    return resultImageView;
}

std::array<VkImageView, 6> OmniShadowResources::CreateCubemapFacesImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R };

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    std::array<VkImageView, 6> shadowCubeMapFaceImageViews{};

    for (uint32_t i = 0; i < 6; i++)
    {
        viewInfo.subresourceRange.baseArrayLayer = i;

        if (vkCreateImageView(device, &viewInfo, nullptr, &shadowCubeMapFaceImageViews[i]) != VK_SUCCESS)
        {
            std::string error = std::string("failed to create cubemap texture[%u] image view!", i);
            throw std::runtime_error(error);
        }
    }

    return shadowCubeMapFaceImageViews;
}

VkSampler OmniShadowResources::CreateCubemapSampler()
{
    VkSampler resultSampler;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(this->deviceModule->device, &samplerInfo, nullptr, &resultSampler))
    {
        throw std::runtime_error("failed to create omni shadow texture sampler!");
    }

    return resultSampler;
}

void OmniShadowResources::TransitionMultiImagesLayout(VkImage newImage, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, commandPool);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = newImage;
    barrier.subresourceRange = subresourceRange;
    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, commandPool, commandBuffer);
}

void OmniShadowResources::CreateOmniShadowMapResources(std::shared_ptr<VkRenderPass> renderPass)
{
    //Cubemap images
    this->cubemapImage = this->AllocateCubemapImage(this->TextureSize, this->TextureSize, this->shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 6;
    this->TransitionMultiImagesLayout(this->cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    this->CubemapSampler = CreateCubemapSampler();
    this->CubemapImageView = this->CreateCubemapImageView(this->deviceModule->device, this->cubemapImage, this->shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    this->cubemapFacesImageViews = this->CreateCubemapFacesImageView(this->deviceModule->device, this->cubemapImage, this->shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    //FRAMEBUFFER OFFSCREEN
    //Depth images
    this->framebufferDepthImage = this->CreateFramebufferDepthImage(this->framebufferDepthImageMemory);

    VkImageSubresourceRange subresourceRangeDepth = {};
    subresourceRangeDepth.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    subresourceRangeDepth.baseMipLevel = 0;
    subresourceRangeDepth.levelCount = 1;
    subresourceRangeDepth.baseArrayLayer = 0;
    subresourceRangeDepth.layerCount = 1;
    this->TransitionMultiImagesLayout(this->framebufferDepthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subresourceRangeDepth);

    this->framebufferDepthImageView = this->CreateImageView(this->deviceModule->device, this->framebufferDepthImage, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    //Framebuffers
    this->PrepareFramebuffers(renderPass, this->framebufferDepthImageView, this->cubemapFacesImageViews);
}

void OmniShadowResources::PrepareFramebuffers(std::shared_ptr<VkRenderPass> renderPass, VkImageView depthView, std::array<VkImageView, 6> cubemapView)
{
    VkImageView attachments[2];
    attachments[1] = depthView;

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = *renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = this->TextureSize;
    fbufCreateInfo.height = this->TextureSize;
    fbufCreateInfo.layers = 1;

    for (uint32_t i = 0; i < 6; i++)
    {
        attachments[0] = cubemapView[i];
        vkCreateFramebuffer(this->deviceModule->device, &fbufCreateInfo, nullptr, &this->CubemapFacesFrameBuffers[i]);
    }
}

void OmniShadowResources::UpdateUBOShadowMap(OmniShadowUniform omniParameters)
{
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(OmniShadowUniform), 0, &data);
        memcpy(data, &omniParameters, sizeof(OmniShadowUniform));
        vkUnmapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}

void OmniShadowResources::Cleanup()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->shadowMapUBO != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->shadowMapUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[i], nullptr);
        }
    }

    if (this->CubemapSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(this->deviceModule->device, this->CubemapSampler, nullptr);
    }
    if (this->CubemapImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, CubemapImageView, nullptr);
    }
    if (this->cubemapImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->cubemapImage, nullptr);
    }
    if (this->cubemapMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, cubemapMemory, nullptr);
    }

    if (this->framebufferDepthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, framebufferDepthImageView, nullptr);
    }
    if (this->framebufferDepthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->framebufferDepthImage, nullptr);
    }
    if (this->framebufferDepthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, framebufferDepthImageMemory, nullptr);
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        vkDestroyImageView(deviceModule->device, this->cubemapFacesImageViews[i], nullptr);
        vkDestroyFramebuffer(this->deviceModule->device, this->CubemapFacesFrameBuffers[i], nullptr);
    }
}
