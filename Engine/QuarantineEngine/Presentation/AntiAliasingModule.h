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
    static void ResetInstance();
    AntiAliasingModule();
    void createColorResources();
    void CleanLastResources();
};

#endif // !ANTIALIASING_MODULE
