#include "CSMResources.h"
#include "ImageMemoryTools.h"
#include "FrameBufferModule.h"
#include <SynchronizationModule.h>
#include <SyncTool.h>

QueueModule* CSMResources::queueModule;
VkCommandPool CSMResources::commandPool;

CSMResources::CSMResources()
{
    this->TextureSize = 1024;
    this->shadowFormat = VK_FORMAT_D32_SFLOAT;
    this->deviceModule = DeviceModule::getInstance();
    this->swapchainModule = SwapChainModule::getInstance();

    this->CascadeResourcesPtr = std::make_shared<std::array<CascadeResource, SHADOW_MAP_CASCADE_COUNT>>();
    this->OffscreenShadowMapUBO = std::make_shared<UniformBufferObject>();
    this->OffscreenShadowMapUBO->CreateUniformBuffer(sizeof(CSMUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

CSMResources::CSMResources(std::shared_ptr<VkRenderPass> renderPass) : CSMResources()
{
    this->CreateCSMResources(renderPass);
}

VkImage CSMResources::AllocateImage(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory& mapMemory, uint32_t pixelSize, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits numSamples)
{
    VkImage result;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { pixelSize, pixelSize, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageInfo, nullptr, &result) != VK_SUCCESS) {
        throw std::runtime_error("failed to create cascade map image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, result, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = IMT::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &mapMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate cascade map image memory!");
    }

    vkBindImageMemory(device, result, mapMemory, 0);

    return result;
}

VkImageView CSMResources::CreateImageView(VkDevice device, VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, int baseArrayLayer, int layerCount, uint32_t mipLevels)
{
    VkImageView resultImageView = {};

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R };

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    viewInfo.subresourceRange.layerCount = layerCount;

    if (vkCreateImageView(device, &viewInfo, nullptr, &resultImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create CSM image view!");
    }

    return resultImageView;
}

VkSampler CSMResources::CreateCSMSampler(VkDevice device)
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &resultSampler))
    {
        throw std::runtime_error("failed to create CSM shadow texture sampler!");
    }

    return resultSampler;
}

void CSMResources::CreateCSMResources(std::shared_ptr<VkRenderPass> renderPass)
{
    this->CSMImage = this->AllocateImage(
        deviceModule->device,
        deviceModule->physicalDevice,
        this->CSMImageMemory,
        this->TextureSize,
        this->shadowFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        1);

    //VkImageSubresourceRange subresourceRange = {};
    //subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //subresourceRange.baseMipLevel = 0;
    //subresourceRange.levelCount = 1;
    //subresourceRange.layerCount = 4;
    //this->TransitionImageLayout(deviceModule->device, this->CSMImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    this->CSMImageView = this->CreateImageView(this->deviceModule->device, this->CSMImage, this->shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 0, SHADOW_MAP_CASCADE_COUNT);
    this->CSMSampler = CreateCSMSampler(this->deviceModule->device);

    for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        this->PrepareFramebuffer(renderPass, i);
    }
}

void CSMResources::PrepareFramebuffer(std::shared_ptr<VkRenderPass> renderPass, int iCascade)
{
    this->CascadeResourcesPtr->at(iCascade).view = CreateImageView(this->deviceModule->device, this->CSMImage, this->shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, iCascade, 1);

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = *renderPass;
    fbufCreateInfo.attachmentCount = 1;
    fbufCreateInfo.pAttachments = &this->CascadeResourcesPtr->at(iCascade).view;
    fbufCreateInfo.width = this->TextureSize;
    fbufCreateInfo.height = this->TextureSize;
    fbufCreateInfo.layers = 1;

    vkCreateFramebuffer(this->deviceModule->device, &fbufCreateInfo, nullptr, &this->CascadeResourcesPtr->at(iCascade).frameBuffer);
}

void CSMResources::UpdateOffscreenUBOShadowMap()
{
    CSMUniform bufferData = {};

    for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        bufferData.cascadeViewProj[i] = this->CascadeResourcesPtr->at(i).viewProjMatrix;
    }

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(CSMUniform), 0, &data);
        memcpy(data, &bufferData, sizeof(CSMUniform));
        vkUnmapMemory(this->deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}

void CSMResources::TransitionImageLayout(VkDevice device, VkImage& newImage, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
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

    if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

    endSingleTimeCommands(device, queueModule->graphicsQueue, commandPool, commandBuffer);
}

void CSMResources::Cleanup()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->OffscreenShadowMapUBO != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffersMemory[i], nullptr);
        }
    }

    if (this->CSMSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(this->deviceModule->device, this->CSMSampler, nullptr);
    }
    if (this->CSMImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, CSMImageView, nullptr);
    }
    if (this->CSMImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->CSMImage, nullptr);
    }
    if (this->CSMImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, CSMImageMemory, nullptr);
    }
  
    for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        this->CascadeResourcesPtr->at(i).destroy(deviceModule->device);
    }
}
