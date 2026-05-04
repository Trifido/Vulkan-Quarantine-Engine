#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

#include <windows.h>
#include <memory>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <filesystem>

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
#include <QECameraContext.h>
#include <CullingSceneManager.h>

enum class ERROR_RESIZE
{
    SWAPCHAIN_ERROR = 0,
    IMAGE_ERROR = 1
};

class QERenderTarget;

class QEBaseApp
{
public:
    QEBaseApp();
    virtual ~QEBaseApp() = default;

    void Run(QEScene scene);

protected:
    virtual void OnInitialize() {}
    virtual void OnShutdown() {}
    virtual void OnFrameStart() {}
    virtual void OnBeginFrame() {}
    virtual void OnEndFrame() {}
    virtual void ConfigureEngineBindings() {}
    virtual void OnPostInitVulkan() {}
    virtual void OnPreCleanup() {}
    virtual void OnSwapchainRecreated() {}
    virtual void OnBeforeSceneActivated() {}
    virtual void OnMainViewportResized(uint32_t width, uint32_t height);
    virtual void RecordAdditionalScenePass(VkCommandBuffer&, uint32_t) {}
    virtual void RecordAdditionalOverlayPass(VkCommandBuffer&, uint32_t) {}

    bool LoadSceneFromPath(const std::filesystem::path& scenePath);
    void UnloadCurrentScene();
    void LoadCurrentScene();

private:
    void InitWindow();
    void initVulkan();
    void loadScene(QEScene& scene);
    void mainLoop();
    void computeFrame(uint32_t currentFrame);
    void drawFrame(uint32_t currentFrame);
    void cleanUp();
    void cleanUpSwapchain();
    void cleanManagers();
    void resizeSwapchain(VkResult result, ERROR_RESIZE errorResize);
    void recreateSwapchain();

protected:
    GUIWindow* mainWindow;

protected:
    VulkanInstance          vulkanInstance{};
    VulkanLayerAndExtension layerExtensionModule{};
    DeviceModule* deviceModule{};
    QueueModule* queueModule{};
    WindowSurface           windowSurface{};
    SwapChainModule* swapchainModule{};
    FramebufferModule       framebufferModule;
    CommandPoolModule* commandPoolModule{};
    SynchronizationModule   synchronizationModule;

    PhysicsModule* physicsModule;

    DepthBufferModule* depthBufferModule;
    AntiAliasingModule* antialiasingModule;
    RenderPassModule* renderPassModule;
    QEDebugSystem* debugSystem;
    bool isRender = false;

    QEScene     scene;

    QECameraContext* cameraContext{};
    LightManager* lightManager{};
    ShaderManager* shaderManager{};
    MaterialManager* materialManager{};
    ComputeNodeManager* computeNodeManager{};
    TextureManager* textureManager{};
    GameObjectManager* gameObjectManager{};
    GraphicsPipelineManager* graphicsPipelineManager{};
    ComputePipelineManager* computePipelineManager{};
    ShadowPipelineManager* shadowPipelineManager{};
    ParticleSystemManager* particleSystemManager{};
    AtmosphereSystem* atmosphereSystem{};

    KeyboardController* keyboard_ptr{};
};


namespace QE
{
    using ::ERROR_RESIZE;
    using ::QERenderTarget;
    using ::QEBaseApp;
} // namespace QE
// QE namespace aliases
