#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

#include <windows.h>
#include <memory>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

//  Kernel
#include "GUIWindow.h"
#include "VulkanInstance.h"
#include "VulkanLayerAndExtension.h"
#include "DeviceModule.h"
#include "WindowSurface.h"
#include "SwapChainModule.h"
#include "QueueModule.h"
#include "FrameBufferModule.h"
#include "CommandPoolModule.h"
#include "SynchronizationModule.h"
#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"
#include "RenderPassModule.h"
#include "OmniShadowResources.h"
#include "Timer.h"

//  Physics System
#include "PhysicsModule.h"

//#include "RayTracingModule.h"

//  Utilities
#include "QEGameObject.h"
#include "QETransform.h"
#include "Material.h"
#include "QECamera.h"
#include "LightManager.h"
#include "Particles/ParticleSystem.h"

// Keyboard controller
#include "KeyboardController.h"

#include <ShaderManager.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <GameObjectManager.h>
#include <ComputePipelineManager.h>
#include <ShadowPipelineManager.h>
#include <Particles/ParticleSystemManager.h>
#include <AtmosphereSystem.h>
#include <CapsuleCollider.h>
#include <QEScene.h>
#include <QESessionManager.h>

enum class ERROR_RESIZE
{
    SWAPCHAIN_ERROR = 0,
    IMAGE_ERROR = 1
};

class QEBaseApp
{
public:
    QEBaseApp();
    virtual ~QEBaseApp() = default;

    void Run(QEScene scene);

    void InitWindow();

protected:
    virtual void OnInitialize() {}
    virtual void OnShutdown() {}
    virtual void OnBeginFrame() {}
    virtual void OnEndFrame() {}
    virtual bool IsEditorMode() const { return false; }
    virtual void OnPostInitVulkan() {}
    virtual void OnPreCleanup() {}
    virtual void OnSwapchainRecreated() {}

private:
    void initVulkan();
    void loadScene(QEScene scene);
    void mainLoop();
    void computeFrame();
    void drawFrame();  
    void cleanUp();
    void cleanUpSwapchain();
    void cleanManagers();
    void resizeSwapchain(VkResult result, ERROR_RESIZE errorResize);
    void recreateSwapchain();
protected:
    GUIWindow*              mainWindow;
public:
    bool                    framebufferResized = false;

protected:
    VulkanInstance          vulkanInstance {};
    VulkanLayerAndExtension layerExtensionModule {};
    DeviceModule*           deviceModule {};
    QueueModule*            queueModule {};
    WindowSurface           windowSurface {};
    SwapChainModule*        swapchainModule {};
    FramebufferModule       framebufferModule;
    CommandPoolModule*      commandPoolModule {};
    SynchronizationModule   synchronizationModule;

    PhysicsModule*          physicsModule;

    DepthBufferModule*      depthBufferModule;
    AntiAliasingModule*     antialiasingModule;
    RenderPassModule*       renderPassModule;
    QEDebugSystem*          debugSystem;

    bool show_demo_window = true;
    bool show_another_window = true;
    bool isRender = false;

    //RayTracingModule        raytracingModule;

    QEScene     scene;

    QESessionManager*   sessionManager{};
    LightManager*       lightManager {};
    ShaderManager*      shaderManager{};
    MaterialManager*    materialManager {};
    ComputeNodeManager* computeNodeManager{};
    TextureManager*     textureManager{};
    GameObjectManager*  gameObjectManager{};
    GraphicsPipelineManager* graphicsPipelineManager{};
    ComputePipelineManager* computePipelineManager{};
    ShadowPipelineManager* shadowPipelineManager{};
    ParticleSystemManager* particleSystemManager{};
    AtmosphereSystem*   atmosphereSystem{};

    KeyboardController* keyboard_ptr {};
};
