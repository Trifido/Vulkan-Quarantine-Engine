#include "EditorViewportResources.h"

#include <stdexcept>
#include <backends/imgui_impl_vulkan.h>

#include <DeviceModule.h>
#include <RenderPassModule.h>
#include <TextureManagerModule.h>
#include <SyncTool.h>
#include <CommandPoolModule.h>
#include <SwapChainModule.h>
#include <DepthBufferModule.h>

EditorViewportResources::EditorViewportResources()
{
}

EditorViewportResources::~EditorViewportResources()
{
}

void EditorViewportResources::Initialize(
    DeviceModule* deviceModule,
    RenderPassModule* renderPassModule,
    CommandPoolModule* commandPoolModule,
    QueueModule* queueModule)
{
    this->deviceModule = deviceModule;
    this->renderPassModule = renderPassModule;
    this->commandPoolModule = commandPoolModule;
    this->queueModule = queueModule;
}

void EditorViewportResources::Cleanup()
{
    UnregisterImGuiTexture();
    CleanupImages();

    renderTarget = {};
    deviceModule = nullptr;
    renderPassModule = nullptr;
    commandPoolModule = nullptr;
    queueModule = nullptr;
}

bool EditorViewportResources::IsValid() const
{
    return renderTarget.Valid() &&
        msaaColorImageView != VK_NULL_HANDLE &&
        resolveImageView != VK_NULL_HANDLE &&
        depthImageView != VK_NULL_HANDLE &&
        imguiDescriptorSet != VK_NULL_HANDLE;
}

bool EditorViewportResources::NeedsResize(uint32_t width, uint32_t height) const
{
    if (width == 0 || height == 0)
    {
        return false;
    }

    return !IsValid() ||
        renderTarget.Extent.width != width ||
        renderTarget.Extent.height != height;
}

void EditorViewportResources::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    vkDeviceWaitIdle(deviceModule->device);

    UnregisterImGuiTexture();
    CleanupImages();

    renderTarget.Extent.width = width;
    renderTarget.Extent.height = height;

    CreateImages();
    CreateFramebuffer();
    RegisterImGuiTexture();
}

void EditorViewportResources::CleanupImages()
{
    if (renderTarget.Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(deviceModule->device, renderTarget.Framebuffer, nullptr);
        renderTarget.Framebuffer = VK_NULL_HANDLE;
    }

    if (resolveSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(deviceModule->device, resolveSampler, nullptr);
        resolveSampler = VK_NULL_HANDLE;
    }

    if (resolveImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, resolveImageView, nullptr);
        resolveImageView = VK_NULL_HANDLE;
    }

    if (resolveImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, resolveImage, nullptr);
        resolveImage = VK_NULL_HANDLE;
    }

    if (resolveMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, resolveMemory, nullptr);
        resolveMemory = VK_NULL_HANDLE;
    }

    if (msaaColorImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, msaaColorImageView, nullptr);
        msaaColorImageView = VK_NULL_HANDLE;
    }

    if (msaaColorImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, msaaColorImage, nullptr);
        msaaColorImage = VK_NULL_HANDLE;
    }

    if (msaaColorMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, msaaColorMemory, nullptr);
        msaaColorMemory = VK_NULL_HANDLE;
    }

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, depthImageView, nullptr);
        depthImageView = VK_NULL_HANDLE;
    }

    if (depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, depthImage, nullptr);
        depthImage = VK_NULL_HANDLE;
    }

    if (depthMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, depthMemory, nullptr);
        depthMemory = VK_NULL_HANDLE;
    }
}

void EditorViewportResources::CreateImages()
{
    renderTarget.RenderPass = *renderPassModule->ViewportRenderPass;

    const uint32_t width = renderTarget.Extent.width;
    const uint32_t height = renderTarget.Extent.height;

    const VkFormat colorFormat = SwapChainModule::getInstance()->swapChainImageFormat;
    const VkFormat depthFormat = DepthBufferModule::getInstance()->findDepthFormat();
    const VkSampleCountFlagBits msaaSamples = *DeviceModule::getInstance()->getMsaaSamples();

    // 1) COLOR MSAA
    {
        TextureManagerModule colorTexture;
        colorTexture.createImage(
            width,
            height,
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            1,
            1,
            msaaSamples);

        msaaColorImage = colorTexture.image;
        msaaColorMemory = colorTexture.deviceMemory;
        colorTexture.image = VK_NULL_HANDLE;
        colorTexture.deviceMemory = VK_NULL_HANDLE;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = msaaColorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = colorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(deviceModule->device, &viewInfo, nullptr, &msaaColorImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create editor viewport MSAA color image view!");
        }
    }

    // 2) DEPTH MSAA
    {
        TextureManagerModule depthTexture;
        depthTexture.createImage(
            width,
            height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            1,
            1,
            msaaSamples);

        depthImage = depthTexture.image;
        depthMemory = depthTexture.deviceMemory;
        depthTexture.image = VK_NULL_HANDLE;
        depthTexture.deviceMemory = VK_NULL_HANDLE;

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image = depthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(deviceModule->device, &depthViewInfo, nullptr, &depthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create editor viewport depth image view!");
        }
    }

    // 3) RESOLVE FINAL
    {
        TextureManagerModule resolveTexture;
        resolveTexture.createImage(
            width,
            height,
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            1,
            1,
            VK_SAMPLE_COUNT_1_BIT);

        resolveImage = resolveTexture.image;
        resolveMemory = resolveTexture.deviceMemory;
        resolveTexture.image = VK_NULL_HANDLE;
        resolveTexture.deviceMemory = VK_NULL_HANDLE;

        VkImageViewCreateInfo resolveViewInfo{};
        resolveViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        resolveViewInfo.image = resolveImage;
        resolveViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        resolveViewInfo.format = colorFormat;
        resolveViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        resolveViewInfo.subresourceRange.baseMipLevel = 0;
        resolveViewInfo.subresourceRange.levelCount = 1;
        resolveViewInfo.subresourceRange.baseArrayLayer = 0;
        resolveViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(deviceModule->device, &resolveViewInfo, nullptr, &resolveImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create editor viewport resolve image view!");
        }

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

        if (vkCreateSampler(deviceModule->device, &samplerInfo, nullptr, &resolveSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create editor viewport resolve sampler!");
        }
    }
}

void EditorViewportResources::CreateFramebuffer()
{
    VkImageView attachments[] =
    {
        msaaColorImageView,
        depthImageView,
        resolveImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderTarget.RenderPass;
    framebufferInfo.attachmentCount = 3;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = renderTarget.Extent.width;
    framebufferInfo.height = renderTarget.Extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(deviceModule->device, &framebufferInfo, nullptr, &renderTarget.Framebuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create editor viewport framebuffer!");
    }
}

void EditorViewportResources::RegisterImGuiTexture()
{
    imguiDescriptorSet = ImGui_ImplVulkan_AddTexture(
        resolveSampler,
        resolveImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void EditorViewportResources::UnregisterImGuiTexture()
{
    if (imguiDescriptorSet != VK_NULL_HANDLE)
    {
        ImGui_ImplVulkan_RemoveTexture(imguiDescriptorSet);
        imguiDescriptorSet = VK_NULL_HANDLE;
    }
}

void EditorViewportResources::TransitionImageLayout(
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange range)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        deviceModule->device,
        commandPoolModule->getCommandPool());

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = range;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::runtime_error("unsupported viewport image layout transition!");
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
        deviceModule->device,
        queueModule->graphicsQueue,
        commandPoolModule->getCommandPool(),
        commandBuffer);
}
