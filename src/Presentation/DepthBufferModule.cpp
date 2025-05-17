#include "DepthBufferModule.h"

DepthBufferModule::DepthBufferModule()
{
    deviceModule = DeviceModule::getInstance();
    queueModule = QueueModule::getInstance();
    antialiasingModule = AntiAliasingModule::getInstance();
}

void DepthBufferModule::createDepthResources(VkExtent2D& swapChainExtent, VkCommandPool& commandPool)
{
    ptrCommandPool = &commandPool;

    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1U, 1U, *antialiasingModule->msaaSamples);

    imageView = IMT::createImageView(deviceModule->device, image, VK_IMAGE_VIEW_TYPE_2D, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, range);
}

VkFormat DepthBufferModule::findDepthFormat()
{
    return deviceModule->findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void DepthBufferModule::CleanLastResources()
{
    this->antialiasingModule = nullptr;
}

bool DepthBufferModule::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
