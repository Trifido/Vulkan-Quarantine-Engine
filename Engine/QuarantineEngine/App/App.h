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
#include "Timer.h"

//  Physics System
#include "PhysicsModule.h"

//#include "RayTracingModule.h"

//  Utilities
#include "GameObject.h"
#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "Camera.h"
#include "LightManager.h"
#include "AnimationManager.h"
#include "Particles/ParticleSystem.h"

// Editor
#include <../Editor/EditorObjectManager.h>
#include <CameraEditor.h>

// Keyboard controller
#include "KeyboardController.h"
#include <ShaderManager.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <GameObjectManager.h>
//#include <Compute/ComputeNodeManager.h>
#include <ComputePipelineManager.h>

const std::string MODEL_PATH = "../../resources/models/head/head.obj";
const std::string TEXTURE_WALL_NORMAL_PATH = "../../resources/textures/wall/brickwall_normal.jpg";
const std::string MODEL_HOUSE_PATH = "../../resources/models/vikingRoom/viking_room.obj";
const std::string MODEL_WHEEL_PATH = "../../resources/models/scifiWheel/source/wheel.glb";
const std::string MODEL_KNIGHT_PATH = "../../resources/models/Bulky Knight/source/Big Night For Sketchfab.fbx";
const std::string MODEL_GUN_PATH = "../../resources/models/Modified Colt/source/Gun Low Poly.fbx";
const std::string MODEL_ATLAS_PATH = "../../resources/models/Atlas/Atlas.obj";
const std::string MODEL_ARTORIAS_PATH = "../../resources/models/Artorias/source/Artorias.fbx";
const std::string MODEL_ZELDA_PATH = "../../resources/models/Zelda/pose.obj";
const std::string MODEL_SURVIVAL_PATH = "../../resources/models/survival/backpack.obj";
const std::string MODEL_CRYSIS_PATH = "../../../resources/models/crysis/nanosuit.obj";
const std::string MODEL_PUMPKIN_PATH = "../../resources/models/pumpkin/pumpkin.obj";

const std::string TEXTURE_PATH = "../../resources/models/head/lambertian.jpg";
const std::string TEXTURE_BUMP_PATH = "../../resources/models/head/bump.png";
const std::string TEXTURE_WALL_PATH = "../../resources/textures/wall/brickwall.jpg";
const std::string TEXTURE_TEST_PATH = "../../resources/textures/test.png";
const std::string TEXTURE_HOUSE_PATH = "../../resources/textures/viking_room.png";

const std::string TEXTURE_CONTAINER_PATH = "../../resources/textures/container2.png";
const std::string TEXTURE_CONTAINERSPEC_PATH = "../../resources/textures/container2_specular.png";

enum class ERROR_RESIZE
{
    SWAPCHAIN_ERROR = 0,
    IMAGE_ERROR = 1
};

class App
{
public:
    App();
    ~App();
    void run();

    void addWindow(GLFWwindow& window);
    void initWindow();
    void init_imgui();
private:
    void initVulkan();
    void mainLoop();
    void computeFrame();
    void drawFrame();  
    void cleanUp();
    void cleanUpSwapchain();
    void cleanManagers();
    void resizeSwapchain(VkResult result, ERROR_RESIZE errorResize);
    void recreateSwapchain();
    //bool createFontsTexture(VkCommandBuffer& commandBuffer);
    //void setupImguiUploadFonts();
public:
    GUIWindow               mainWindow;
    bool                    framebufferResized = false;
private:
    uint32_t     imageIndex;
    VulkanInstance          vulkanInstance {};
    VulkanLayerAndExtension layerExtensionModule {};
    DeviceModule*           deviceModule {};
    QueueModule*            queueModule {};
    WindowSurface           windowSurface {};
    SwapChainModule*        swapchainModule {};
    FramebufferModule       framebufferModule;
    CommandPoolModule*      commandPoolModule {};
    SynchronizationModule   synchronizationModule;
    VkDescriptorPool        imguiPool {};

    PhysicsModule*          physicsModule;

    DepthBufferModule*      depthBufferModule;
    AntiAliasingModule*     antialiasingModule;
    RenderPassModule*       renderPassModule;
    Timer*                  timer;

    bool show_demo_window = true;
    bool show_another_window = true;
    //FontResourcesModule     fontModule;

    //RayTracingModule        raytracingModule;

    Camera*     cameraEditor;
    LightManager*       lightManager {};
    ShaderManager*      shaderManager{};
    MaterialManager*    materialManager {};
    ComputeNodeManager* computeNodeManager{};
    TextureManager*     textureManager{};
    GameObjectManager*  gameObjectManager{};
    AnimationManager*   animationManager{};
    GraphicsPipelineManager* graphicsPipelineManager{};
    ComputePipelineManager* computePipelineManager{};

    KeyboardController* keyboard_ptr {};

    // Editor
    EditorObjectManager* editorManager {};
};
