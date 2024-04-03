#include "FrameBufferModule.h"

#include <stdexcept>

FramebufferModule::FramebufferModule()
{
    this->deviceModule = DeviceModule::getInstance();
    this->antialiasingModule = AntiAliasingModule::getInstance();
    this->swapchainModule = SwapChainModule::getInstance();
    this->depthbufferModule = DepthBufferModule::getInstance();
    this->shadowMappingModule = ShadowMappingModule::getInstance();
}

void FramebufferModule::createFramebuffer(VkRenderPass& renderPass)
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
        framebufferInfo.renderPass = renderPass;
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

void FramebufferModule::createShadowFramebuffer(VkRenderPass& renderPass)
{
    // Create frame buffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &(this->shadowMappingModule->imageView);
    framebufferInfo.width = this->shadowMappingModule->TextureSize;
    framebufferInfo.height = this->shadowMappingModule->TextureSize;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(deviceModule->device, &framebufferInfo, nullptr, &shadowMapFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow map framebuffer!");
    }
}

void FramebufferModule::cleanup()
{
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(deviceModule->device, framebuffer, nullptr);
    }
}
