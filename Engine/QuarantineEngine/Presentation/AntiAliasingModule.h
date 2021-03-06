#pragma once
#ifndef ANTIALIASING_MODULE
#define ANTIALIASING_MODULE

#include <vulkan/vulkan.h>

#include "TextureManagerModule.h"
#include "SwapChainModule.h"

class AntiAliasingModule : public TextureManagerModule
{
public:
    VkSampleCountFlagBits*   msaaSamples;

public:
    AntiAliasingModule();
    void createColorResources(SwapChainModule& swapChainModule);
};

#endif // !ANTIALIASING_MODULE
