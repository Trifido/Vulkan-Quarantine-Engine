#include "EditorViewportResources.h"

#include <stdexcept>
#include <backends/imgui_impl_vulkan.h>

#include <DeviceModule.h>
#include <RenderPassModule.h>
#include <TextureManagerModule.h>

EditorViewportResources::EditorViewportResources()
{
}

EditorViewportResources::~EditorViewportResources()
{
}

void EditorViewportResources::Initialize(DeviceModule* deviceModule, RenderPassModule* renderPassModule)
{
    this->deviceModule = deviceModule;
    this->renderPassModule = renderPassModule;
}

void EditorViewportResources::Cleanup()
{
    UnregisterImGuiTexture();
    CleanupImages();

    renderTarget = {};
    deviceModule = nullptr;
    renderPassModule = nullptr;
}

bool EditorViewportResources::IsValid() const
{
    return renderTarget.Valid() &&
        colorImageView != VK_NULL_HANDLE &&
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

    if (colorSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(deviceModule->device, colorSampler, nullptr);
        colorSampler = VK_NULL_HANDLE;
    }

    if (colorImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, colorImageView, nullptr);
        colorImageView = VK_NULL_HANDLE;
    }

    if (colorImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, colorImage, nullptr);
        colorImage = VK_NULL_HANDLE;
    }

    if (colorMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, colorMemory, nullptr);
        colorMemory = VK_NULL_HANDLE;
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
    renderTarget.RenderPass = *renderPassModule->DefaultRenderPass;

    const uint32_t width = renderTarget.Extent.width;
    const uint32_t height = renderTarget.Extent.height;

    // COLOR
    TextureManagerModule colorTexture;
    colorTexture.createImage(
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT);

    colorImage = colorTexture.image;
    colorMemory = colorTexture.deviceMemory;
    colorTexture.image = VK_NULL_HANDLE;
    colorTexture.deviceMemory = VK_NULL_HANDLE;

    VkImageViewCreateInfo colorViewInfo{};
    colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorViewInfo.image = colorImage;
    colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorViewInfo.subresourceRange.baseMipLevel = 0;
    colorViewInfo.subresourceRange.levelCount = 1;
    colorViewInfo.subresourceRange.baseArrayLayer = 0;
    colorViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(deviceModule->device, &colorViewInfo, nullptr, &colorImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create editor viewport color image view!");
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

    if (vkCreateSampler(deviceModule->device, &samplerInfo, nullptr, &colorSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create editor viewport sampler!");
    }

    // DEPTH
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

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
        VK_SAMPLE_COUNT_1_BIT);

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

    // Transiciones iniciales mínimas
    TextureManagerModule transitionHelper;
    transitionHelper.transitionImageLayout(
        depthImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 });

    // OJO:
    // la color image para offscreen idealmente debería terminar en
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL al acabar el render pass.
    // De momento no la transicionamos aquí porque dependerá del render pass.
}
void EditorViewportResources::CreateFramebuffer()
{
    VkImageView attachments[] =
    {
        colorImageView,
        depthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderTarget.RenderPass;
    framebufferInfo.attachmentCount = 2;
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
        colorSampler,
        colorImageView,
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
