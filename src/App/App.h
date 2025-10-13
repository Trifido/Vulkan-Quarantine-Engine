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

using namespace std;

const string MODEL_PATH = "../../resources/models/head/head.obj";
const string TEXTURE_WALL_NORMAL_PATH = "../../resources/textures/wall/brickwall_normal.jpg";
const string MODEL_HOUSE_PATH = "../../resources/models/vikingRoom/viking_room.obj";
const string MODEL_WHEEL_PATH = "../../resources/models/scifiWheel/source/wheel.glb";
const string MODEL_KNIGHT_PATH = "../../resources/models/Bulky Knight/source/Big Night For Sketchfab.fbx";
const string MODEL_GUN_PATH = "../../resources/models/Modified Colt/source/Gun Low Poly.fbx";
const string MODEL_ATLAS_PATH = "../../resources/models/Atlas/Atlas.obj";
const string MODEL_ARTORIAS_PATH = "../../resources/models/Artorias/source/Artorias.fbx";
const string MODEL_ZELDA_PATH = "../../resources/models/Zelda/pose.obj";
const string MODEL_SURVIVAL_PATH = "../../resources/models/survival/backpack.obj";
const string MODEL_CRYSIS_PATH = "../../../resources/models/crysis/nanosuit.obj";
const string MODEL_PUMPKIN_PATH = "../../resources/models/pumpkin/pumpkin.obj";

const string TEXTURE_PATH = "../../resources/models/head/lambertian.jpg";
const string TEXTURE_BUMP_PATH = "../../resources/models/head/bump.png";
const string TEXTURE_WALL_PATH = "../../resources/textures/wall/brickwall.jpg";
const string TEXTURE_TEST_PATH = "../../resources/textures/test.png";
const string TEXTURE_HOUSE_PATH = "../../resources/textures/viking_room.png";
const string TEXTURE_SKYBOX_PATH = "skybox/HDRIHaven.jpg";
const vector<string> TEXTURE_SKYBOX_PATH_FACES = { "skybox/right.jpg", "skybox/left.jpg", "skybox/top.jpg", "skybox/bottom.jpg", "skybox/front.jpg", "skybox/back.jpg" };
const string TEXTURE_SPHERICAL_MAP_PATH = "skybox/klippad_dawn_1_4k.hdr";

const string TEXTURE_CONTAINER_PATH = "../../resources/textures/container2.png";
const string TEXTURE_CONTAINERSPEC_PATH = "../../resources/textures/container2_specular.png";

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
    void run(QEScene scene, bool isEditorMode);

    void initWindow();
    void init_imgui();
private:
    void initVulkan();
    void loadScene(QEScene scene);
    void saveScene();
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
    GUIWindow*              mainWindow;
    bool                    framebufferResized = false;

private:
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
    bool isRender = false;
    bool isRunEditor = true;
    //FontResourcesModule     fontModule;

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
