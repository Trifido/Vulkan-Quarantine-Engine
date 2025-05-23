#include "AntiAliasingModule.h"

void AntiAliasingModule::createColorResources()
{
    VkFormat colorFormat = this->swapchainModule->swapChainImageFormat;
    createImage(this->swapchainModule->swapChainExtent.width, this->swapchainModule->swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1U, 1U, *msaaSamples);

    imageView = IMT::createImageView(deviceModule->device, image, VK_IMAGE_VIEW_TYPE_2D, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void AntiAliasingModule::CleanLastResources()
{
    this->msaaSamples = nullptr;
}

AntiAliasingModule::AntiAliasingModule()
{
    msaaSamples = deviceModule->getMsaaSamples();
}
