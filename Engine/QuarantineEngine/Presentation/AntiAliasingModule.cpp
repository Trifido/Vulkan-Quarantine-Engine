#include "AntiAliasingModule.h"

void AntiAliasingModule::createColorResources(SwapChainModule& swapChainModule)
{
    VkFormat colorFormat = swapChainModule.swapChainImageFormat;
    createImage(swapChainModule.swapChainExtent.width, swapChainModule.swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, *msaaSamples);

    imageView = IMT::createImageView(deviceModule->device, image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

AntiAliasingModule::AntiAliasingModule()
{
    msaaSamples = deviceModule->getMsaaSamples();
}
