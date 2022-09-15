#pragma once
#ifndef ANTIALIASING_MODULE
#define ANTIALIASING_MODULE

#include <vulkan/vulkan.hpp>

#include "TextureManagerModule.h"

class AntiAliasingModule : public TextureManagerModule
{
public:
    static AntiAliasingModule* instance;
    VkSampleCountFlagBits*  msaaSamples = nullptr;
public:
    static AntiAliasingModule* getInstance();
    AntiAliasingModule();
    void createColorResources();
};

#endif // !ANTIALIASING_MODULE
