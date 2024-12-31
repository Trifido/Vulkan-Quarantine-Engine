#pragma once
#ifndef ANTIALIASING_MODULE
#define ANTIALIASING_MODULE

#include <vulkan/vulkan.hpp>

#include "TextureManagerModule.h"
#include <QESingleton.h>

class AntiAliasingModule : public TextureManagerModule, public QESingleton<AntiAliasingModule>
{
private:
    friend class QESingleton<AntiAliasingModule>; // Permitir acceso al constructor

public:
    VkSampleCountFlagBits*  msaaSamples = nullptr;

public:
    AntiAliasingModule();
    void createColorResources();
    void CleanLastResources();
};

#endif // !ANTIALIASING_MODULE
