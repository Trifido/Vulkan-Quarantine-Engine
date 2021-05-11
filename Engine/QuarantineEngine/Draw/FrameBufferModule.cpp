#include "FrameBufferModule.h"

#include <stdexcept>

FramebufferModule::FramebufferModule()
{
    deviceModule = DeviceModule::getInstance();
}

void FramebufferModule::addAntialiasingModule(AntiAliasingModule& antialiasingModule)
{
    antialias_ptr = &antialiasingModule;
}

void FramebufferModule::createFramebuffer(VkRenderPass& renderPass, std::vector<VkImageView>& swapChainImageViews, VkExtent2D& swapChainExtent, DepthBufferModule& depthBufferModule)
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 3> attachments = {
            antialias_ptr->imageView,
            depthBufferModule.imageView,
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
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
