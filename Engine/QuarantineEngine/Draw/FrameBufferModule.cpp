#include "FrameBufferModule.h"

#include <stdexcept>

FramebufferModule::FramebufferModule()
{
    deviceModule = DeviceModule::getInstance();
    antialiasingModule = AntiAliasingModule::getInstance();
    swapchainModule = SwapChainModule::getInstance();
    depthbufferModule = DepthBufferModule::getInstance();
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

void FramebufferModule::cleanup()
{
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(deviceModule->device, framebuffer, nullptr);
    }
}
