#include "ImageViewModule.h"
#include "ImageMemoryTools.h"
#include <stdexcept>

ImageViewModule::ImageViewModule()
{
    deviceModule = DeviceModule::getInstance();
}

void ImageViewModule::createImageViews(SwapChainModule& swapChainModule)
{
    swapChainImageViews.resize(swapChainModule.getNumSwapChainImages());

    for (size_t i = 0; i < swapChainModule.getNumSwapChainImages(); i++)
    {
        swapChainImageViews[i] = IMT::createImageView(deviceModule->device, swapChainModule.swapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, swapChainModule.swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void ImageViewModule::cleanup()
{
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(deviceModule->device, imageView, nullptr);
    }
}
