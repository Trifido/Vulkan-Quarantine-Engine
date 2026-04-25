#include "SpotShadowResources.h"
#include <ImageMemoryTools.h>
#include <stdexcept>
#include <Helpers/QEMemoryTrack.h>
#include <SynchronizationModule.h>

uint32_t SpotShadowResources::TextureSize = 2048;

SpotShadowResources::SpotShadowResources()
{
    this->deviceModule = DeviceModule::getInstance();
    this->swapchainModule = SwapChainModule::getInstance();
    this->shadowFormat = CSMResources::GetSupportedShadowFormat(this->deviceModule);

    this->OffscreenShadowMapUBO = std::make_shared<UniformBufferObject>();
    this->OffscreenShadowMapUBO->CreateUniformBuffer(sizeof(CSMUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

SpotShadowResources::SpotShadowResources(std::shared_ptr<VkRenderPass> renderPass) : SpotShadowResources()
{
    this->CreateSpotShadowResources(renderPass);
}

void SpotShadowResources::CreateSpotShadowResources(std::shared_ptr<VkRenderPass> renderPass)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { TextureSize, TextureSize, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = this->shadowFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(deviceModule->device, &imageInfo, nullptr, &this->shadowImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create spot shadow image!");
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(deviceModule->device, this->shadowImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = IMT::findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        deviceModule->physicalDevice);

    if (vkAllocateMemory(deviceModule->device, &allocInfo, nullptr, &this->shadowImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate spot shadow image memory!");
    }

    vkBindImageMemory(deviceModule->device, this->shadowImage, this->shadowImageMemory, 0);

    CSMResources::TransitionImageLayout(
        deviceModule->device,
        this->shadowImage,
        this->shadowFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        1);

    this->ShadowImageView = IMT::createImageView(
        deviceModule->device,
        this->shadowImage,
        VK_IMAGE_VIEW_TYPE_2D,
        this->shadowFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    this->ShadowSampler = CSMResources::CreateCSMSampler(deviceModule->device);

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = *renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &this->ShadowImageView;
    framebufferInfo.width = TextureSize;
    framebufferInfo.height = TextureSize;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(deviceModule->device, &framebufferInfo, nullptr, &this->frameBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create spot shadow framebuffer!");
    }
}

void SpotShadowResources::UpdateOffscreenUBOShadowMap()
{
    CSMUniform bufferData{};
    bufferData.cascadeViewProj[0] = this->ViewProjMatrix;
    bufferData.cascadeViewProj[1] = glm::mat4(1.0f);
    bufferData.cascadeViewProj[2] = glm::mat4(1.0f);
    bufferData.cascadeViewProj[3] = glm::mat4(1.0f);

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data = nullptr;
        vkMapMemory(
            this->deviceModule->device,
            this->OffscreenShadowMapUBO->uniformBuffersMemory[currentFrame],
            0,
            sizeof(CSMUniform),
            0,
            &data);
        memcpy(data, &bufferData, sizeof(CSMUniform));
        vkUnmapMemory(this->deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}

void SpotShadowResources::Cleanup()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->OffscreenShadowMapUBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffers[i], "SpotShadowResources::Cleanup");
            QE_FREE_MEMORY(deviceModule->device, this->OffscreenShadowMapUBO->uniformBuffersMemory[i], "SpotShadowResources::Cleanup");
        }
    }

    if (this->frameBuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(deviceModule->device, this->frameBuffer, nullptr);
    }

    if (this->ShadowSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(deviceModule->device, this->ShadowSampler, nullptr);
    }

    if (this->ShadowImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, this->ShadowImageView, nullptr);
    }

    if (this->shadowImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, this->shadowImage, nullptr);
    }

    if (this->shadowImageMemory != VK_NULL_HANDLE)
    {
        QE_FREE_MEMORY(deviceModule->device, this->shadowImageMemory, "SpotShadowResources::Cleanup");
    }
}
