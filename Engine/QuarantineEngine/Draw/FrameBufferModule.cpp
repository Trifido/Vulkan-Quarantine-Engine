#include "FrameBufferModule.h"

#include <stdexcept>

FramebufferModule::FramebufferModule()
{
    this->deviceModule = DeviceModule::getInstance();
    this->antialiasingModule = AntiAliasingModule::getInstance();
    this->swapchainModule = SwapChainModule::getInstance();
    this->depthbufferModule = DepthBufferModule::getInstance();
}

void FramebufferModule::createFramebuffer(std::shared_ptr<VkRenderPass> renderPass)
{
    size_t numSwapchainImageViews = swapchainModule->swapChainImageViews.size();
    swapChainFramebuffers.resize(numSwapchainImageViews);

    for (size_t i = 0; i < numSwapchainImageViews; i++)
    {
        std::array<VkImageView, 3> attachments = {
            antialiasingModule->imageView,
            depthbufferModule->imageView,
            swapchainModule->swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainModule->swapChainExtent.width;
        framebufferInfo.height = swapchainModule->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(deviceModule->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

VkFramebuffer FramebufferModule::CreateShadowFramebuffer(std::shared_ptr<VkRenderPass> renderPass, VkImageView& imageView, uint32_t textureSize, VkDevice& device)
{
    VkFramebuffer result;

    // Create frame buffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = *renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &imageView;
    framebufferInfo.width = textureSize;
    framebufferInfo.height = textureSize;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &result) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map framebuffer!");
    }

    return result;
}

std::array<VkFramebuffer, 6> FramebufferModule::CreateOmniShadowFramebuffer(std::shared_ptr<VkRenderPass> renderPass, VkImageView& depthImageView, std::array<VkImageView, 6> imagesView, uint32_t textureSize, VkDevice& device)
{
    VkImageView attachments[2];
    attachments[1] = depthImageView;

    std::array<VkFramebuffer, 6> result;

    // Create frame buffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = *renderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = textureSize;
    framebufferInfo.height = textureSize;
    framebufferInfo.layers = 1;

    for (uint32_t i = 0; i < 6; i++)
    {
        attachments[0] = imagesView[i];

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &result[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shadow map framebuffer!");
        }
    }

    return result;
}

void FramebufferModule::cleanup()
{
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(deviceModule->device, framebuffer, nullptr);
    }
}

//void FramebufferModule::cleanupShadowBuffer()
//{
//    vkDestroyFramebuffer(deviceModule->device, shadowMapFramebuffer, nullptr);
//}
