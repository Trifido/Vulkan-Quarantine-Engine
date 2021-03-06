#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "GUIWindow.h"
#include "VulkanInstance.h"
#include "VulkanLayerAndExtension.h"
#include "DeviceModule.h"
#include "WindowSurface.h"
#include "SwapChainModule.h"
#include "QueueModule.h"
#include "ImageViewModule.h"
#include "GraphicsPipelineModule.h"
#include "FrameBufferModule.h"
#include "CommandPoolModule.h"
#include "SynchronizationModule.h"
#include "GeometryModule.h"
#include "BufferManageModule.h"
#include "TextureManagerModule.h"
#include "TextureModule.h"
#include "DescriptorModule.h"
#include "DepthBufferModule.h"

enum class ERROR_RESIZE
{
    SWAPCHAIN_ERROR = 0,
    IMAGE_ERROR = 1
};

class App
{
public:
    void run();
private:
    void initWindow();
    void initVulkan();
    void mainLoop();  
    void drawFrame();  
    void cleanUp();
    void cleanUpSwapchain();
    void resizeSwapchain(VkResult result, ERROR_RESIZE errorResize);
    void recreateSwapchain();

public:
    GUIWindow               mainWindow;
    bool                    framebufferResized = false;
private:
    VulkanInstance          vulkanInstance;
    VulkanLayerAndExtension layerExtensionModule;
    DeviceModule*           deviceModule;
    QueueModule             queueModule;
    WindowSurface           windowSurface;
    SwapChainModule         swapchainModule;
    ImageViewModule         imageViewModule;
    ShaderModule            shaderModule;
    GraphicsPipelineModule  graphicsPipelineModule;
    FramebufferModule       framebufferModule;
    CommandPoolModule*      commandPoolModule;
    SynchronizationModule   synchronizationModule;
    GeometryModule          geometryModule;
    BufferManageModule      bufferModule;
    DescriptorModule        descriptorModule;
    TextureModule           textureModule;
    DepthBufferModule       depthBufferModule;
};

