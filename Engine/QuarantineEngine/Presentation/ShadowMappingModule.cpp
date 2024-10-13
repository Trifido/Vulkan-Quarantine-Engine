#include "ShadowMappingModule.h"
#include "ImageMemoryTools.h"
#include "FrameBufferModule.h"
#include <SyncTool.h>

VkCommandPool ShadowMappingModule::commandPool;

ShadowMappingModule::ShadowMappingModule()
{
    this->TextureSize = 1024;
    this->shadowFormat = VK_FORMAT_D32_SFLOAT;
    this->ptrCommandPool = &commandPool;
}

ShadowMappingModule::ShadowMappingModule(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass, ShadowMappingMode mode) : ShadowMappingModule()
{
    this->shaderModule = shaderModule;
    this->shadowPipelineModule = this->shaderModule->ShadowPipelineModule;
    this->shadowMode = mode;

    switch (this->shadowMode)
    {
        default:
        case ShadowMappingMode::NONE:
            break;
        case ShadowMappingMode::DIRECTIONAL_SHADOW:
            this->CreateDirectionalShadowMapResources(renderPass);
            break;
        case ShadowMappingMode::OMNI_SHADOW:
            this->shadowFormat = VK_FORMAT_R32_SFLOAT;
            this->CreateOmniShadowMapResources(renderPass);
            break;
        case ShadowMappingMode::CASCADE_SHADOW:
            break;
    }
}

void ShadowMappingModule::CreateDirectionalShadowMapResources(std::shared_ptr<VkRenderPass> renderPass)
{
    this->createImage(this->TextureSize, this->TextureSize, this->shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
    this->imageView = IMT::createImageView(this->deviceModule->device, this->image, this->shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if(vkCreateSampler(this->deviceModule->device, &samplerInfo, nullptr, &this->depthSampler))
    {
        throw std::runtime_error("failed to create shadow texture sampler!");
    }

    this->shadowFrameBuffer = FramebufferModule::CreateShadowFramebuffer(renderPass, this->imageView, this->TextureSize, this->deviceModule->device);
}

void ShadowMappingModule::CreateOmniShadowMapResources(std::shared_ptr<VkRenderPass> renderPass)
{
    this->createCubeMapImage(this->TextureSize, this->TextureSize, this->shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);

    // Image barrier for optimal image (target)
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 6;
    this->transitionMultiImagesLayout(this->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

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

    if (vkCreateSampler(this->deviceModule->device, &samplerInfo, nullptr, &this->depthSampler))
    {
        throw std::runtime_error("failed to create omni shadow texture sampler!");
    }

    this->createCubeMapImageViews = IMT::createCubeMapImageView(this->deviceModule->device, this->image, this->shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    this->PrepareFramebuffers(renderPass);
}

void ShadowMappingModule::PrepareFramebuffers(std::shared_ptr<VkRenderPass> renderPass)
{
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

    ////VkImageViewCreateInfo colorImageView = {};
    ////colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ////colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ////colorImageView.format = this->shadowFormat;
    ////colorImageView.flags = 0;
    ////colorImageView.subresourceRange = {};
    ////colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ////colorImageView.subresourceRange.baseMipLevel = 0;
    ////colorImageView.subresourceRange.levelCount = 1;
    ////colorImageView.subresourceRange.baseArrayLayer = 0;
    ////colorImageView.subresourceRange.layerCount = 1;

    // Depth stencil attachment
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    depthStencilView.flags = 0;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;

    vkCreateImage(this->deviceModule->device, &imageCreateInfo, nullptr, &depthImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(this->deviceModule->device, depthImage, &memReqs);

    VkMemoryAllocateInfo memAlloc {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = IMT::findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceModule->physicalDevice);
    vkAllocateMemory(this->deviceModule->device, &memAlloc, nullptr, &deviceDepthMemory);
    vkBindImageMemory(this->deviceModule->device, depthImage, deviceDepthMemory, 0);

    this->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    depthStencilView.image = depthImage;

    vkCreateImageView(this->deviceModule->device, &depthStencilView, nullptr, &imageView);

    VkImageView attachments[2];
    attachments[1] = imageView;

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
        attachments[0] = this->createCubeMapImageViews[i];
        vkCreateFramebuffer(this->deviceModule->device, &fbufCreateInfo, nullptr, &this->shadowFrameBuffers[i]);
    }
}

void ShadowMappingModule::cleanup()
{
    TextureManagerModule::cleanup();

    vkDestroySampler(this->deviceModule->device, this->depthSampler, nullptr);
    vkDestroyFramebuffer(this->deviceModule->device, this->shadowFrameBuffer, nullptr);

    if (this->depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->depthImage, nullptr);
    }

    if (this->deviceDepthMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, deviceDepthMemory, nullptr);
    }

    for (uint32_t i = 0; i < 6; i++)
    {
        vkDestroyImageView(deviceModule->device, this->createCubeMapImageViews[i], nullptr);
        vkDestroyFramebuffer(this->deviceModule->device, this->shadowFrameBuffers[i], nullptr);
    }
}
