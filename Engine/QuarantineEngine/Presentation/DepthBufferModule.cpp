#include "DepthBufferModule.h"

DepthBufferModule* DepthBufferModule::instance = nullptr;

DepthBufferModule* DepthBufferModule::getInstance()
{
	if (instance == NULL)
		instance = new DepthBufferModule();

	return instance;
}

void DepthBufferModule::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

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

    imageView = IMT::createImageView(deviceModule->device, image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
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
