#pragma once
#ifndef IMAGE_VIEW_MODULE_H
#define IMAGE_VIEW_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>

#include "SwapChainModule.h"

class ImageViewModule
{
private:
    DeviceModule* deviceModule;
public:
    std::vector<VkImageView> swapChainImageViews;

public:
    ImageViewModule();
    void createImageViews(SwapChainModule &swapChainModule);
    void cleanup();
};

#endif
