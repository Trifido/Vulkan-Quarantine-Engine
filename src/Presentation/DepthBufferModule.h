#pragma once

#ifndef DEPTH_BUFFER_MODULE_H
#define DEPTH_BUFFER_MODULE_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <vector>
#include <stdexcept>

#include "TextureManagerModule.h"
#include "SwapChainModule.h"
#include "AntiAliasingModule.h"
#include <QESingleton.h>

class DepthBufferModule : public TextureManagerModule, public QESingleton<DepthBufferModule>
{
private:
    friend class QESingleton<DepthBufferModule>; // Permitir acceso al constructor
    AntiAliasingModule* antialiasingModule;

public:
    DepthBufferModule();
    void createDepthResources(VkExtent2D& swapChainExtent, VkCommandPool& commandPool);
    VkFormat findDepthFormat();
    void CleanLastResources();
private:
    bool hasStencilComponent(VkFormat format);
};

#endif
