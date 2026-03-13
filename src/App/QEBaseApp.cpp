#include "QEBaseApp.h"

#include "SyncTool.h"

#include <BufferManageModule.h>
#include <filesystem>
#include <QEProjectManager.h>
#include <QEMeshRenderer.h>
#include <QESpringArmComponent.h>
#include <PlaneCollider.h>
#include <BoxCollider.h>
#include "PhysicsBody.h"

QEBaseApp::QEBaseApp()
{
    this->keyboard_ptr = KeyboardController::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->deviceModule = DeviceModule::getInstance();

    this->debugSystem = QEDebugSystem::getInstance();
    this->physicsModule = PhysicsModule::getInstance();

    this->graphicsPipelineManager = GraphicsPipelineManager::getInstance();
    this->shadowPipelineManager = ShadowPipelineManager::getInstance();
    this->computePipelineManager = ComputePipelineManager::getInstance();
}

void QEBaseApp::Run(QEScene scene)
{
    this->scene = scene;

    InitWindow();
    initVulkan();

    OnInitialize();

    mainLoop();

    OnShutdown();
    cleanUp();
}

void QEBaseApp::InitWindow()
{
    this->mainWindow = GUIWindow::getInstance();
    this->mainWindow->init();
}

void QEBaseApp::initVulkan()
{
    vulkanInstance.debug_level = DEBUG_LEVEL::ONLY_ERROR;
    vulkanInstance.createInstance();
    layerExtensionModule.setupDebugMessenger(vulkanInstance.getInstance(), vulkanInstance.debug_level);
    windowSurface.createSurface(vulkanInstance.getInstance(), mainWindow->getWindow());
    deviceModule->pickPhysicalDevice(vulkanInstance.getInstance(), windowSurface.getSurface());
    deviceModule->createLogicalDevice(windowSurface.getSurface(), *queueModule);

    //Inicializamos el CommandPool Module
    commandPoolModule = CommandPoolModule::getInstance();
    commandPoolModule->ClearColor = glm::vec3(0.1f);

    //Inicializamos el Swapchain Module
    swapchainModule = SwapChainModule::getInstance();
    swapchainModule->InitializeScreenDataResources();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow->getWindow());

    //Creamos el Command pool module y los Command buffers
    commandPoolModule->createCommandPool(windowSurface.getSurface());
    commandPoolModule->createCommandBuffers();

    //Creamos el antialiasing module
    antialiasingModule = AntiAliasingModule::getInstance();
    antialiasingModule->createColorResources();

    //Creamos el depth buffer module
    depthBufferModule = DepthBufferModule::getInstance();
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Creamos el Render Pass
    renderPassModule = RenderPassModule::getInstance();
    renderPassModule->CreateRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);
    renderPassModule->CreateDirShadowRenderPass(VK_FORMAT_D32_SFLOAT);
    renderPassModule->CreateOmniShadowRenderPass(VK_FORMAT_R32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

    //Registramos el default render pass
    this->graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->DefaultRenderPass);

    //Creamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->DefaultRenderPass);

    BufferManageModule::commandPool = this->commandPoolModule->getCommandPool();
    BufferManageModule::computeCommandPool = this->commandPoolModule->getComputeCommandPool();
    BufferManageModule::graphicsQueue = this->queueModule->graphicsQueue;
    BufferManageModule::computeQueue = this->queueModule->computeQueue;
    QEGeometryComponent::deviceModule_ptr = this->deviceModule;
    TextureManagerModule::queueModule = this->queueModule;
    CustomTexture::commandPool = commandPoolModule->getCommandPool();
    OmniShadowResources::commandPool = commandPoolModule->getCommandPool();
    CSMResources::commandPool = commandPoolModule->getCommandPool();
    OmniShadowResources::queueModule = this->queueModule;
    CSMResources::queueModule = this->queueModule;

    // INIT ------------------------- Managers -------------------------------
    this->sessionManager = QESessionManager::getInstance();

    this->shaderManager = ShaderManager::getInstance();
    this->textureManager = TextureManager::getInstance();
    this->lightManager = LightManager::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->materialManager->InitializeMaterialManager();
    this->gameObjectManager = GameObjectManager::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();
    this->computeNodeManager->InitializeComputeResources();
    this->particleSystemManager = ParticleSystemManager::getInstance();
    this->debugSystem = QEDebugSystem::getInstance();
    this->debugSystem->InitializeDebugGraphicResources();

    // Import meshes
    //QEProjectManager::ImportMeshFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Character/Character.glb");

    // Import animations
    //QEProjectManager::ImportAnimationFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Character/Walking.glb",
    //    std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Character/Animations"));

    //QEProjectManager::ImportMeshFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/FuturisticRoom/sci-fi_lab.glb");
    //auto streetPath = std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/FuturisticRoom/Meshes/sci-fi_lab.gltf").generic_string();
    // Load Scene
    this->loadScene(this->scene);

    //auto materialBalPath = std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/MaterialBall2/Meshes/3d_material_ball.gltf").generic_string();
    //std::shared_ptr<QEGameObject> ball = std::make_shared<QEGameObject>("ball");
    //ball->AddComponent(std::make_shared<QEGeometryComponent>(std::make_unique<MeshGenerator>(materialBalPath)));
    //ball->AddComponent(std::make_shared<QEMeshRenderer>());
    //this->gameObjectManager->AddGameObject(ball);

    this->physicsModule->SetGravity(-20.0f);
    this->lightManager->InitializeShadowMaps();
    this->atmosphereSystem->InitializeAtmosphereResources();
    this->synchronizationModule.createSyncObjects();

    OnPostInitVulkan();
}

void QEBaseApp::loadScene(QEScene scene)
{
    scene.DeserializeScene();

    this->gameObjectManager->StartQEGameObjects();
    this->sessionManager->RegisterActiveSceneCamera();

    this->sessionManager->SetEditorMode(IsEditorMode());
    this->sessionManager->SetDebugMode(false);

    if (IsEditorMode())
    {
        this->sessionManager->SetupEditor();
    }

    this->sessionManager->ActiveCamera()->QEStart();

    this->lightManager->AddDirShadowMapShader(materialManager->csm_shader);
    this->lightManager->AddOmniShadowMapShader(materialManager->omni_shadow_mapping_shader);

    this->atmosphereSystem = AtmosphereSystem::getInstance();
    this->atmosphereSystem->LoadAtmosphereDto(scene.atmosphereDto);
}

void QEBaseApp::mainLoop()
{
    while (!glfwWindowShouldClose(mainWindow->getWindow()))
    {
        glfwPollEvents();

        Timer::getInstance()->UpdateDeltaTime();

        this->debugSystem->ClearLines();

        // Start GameObjects 
        this->gameObjectManager->StartQEGameObjects();

        //PHYSIC SYSTEM
        int physicsSteps = Timer::getInstance()->ComputeFixedSteps();
        for (int i = 0; i < physicsSteps; ++i)
            physicsModule->ComputePhysics(Timer::getInstance()->FixedDelta);
        physicsModule->UpdateDebugPhysicsDrawer();

        // UPDATE GameObjects 
        this->gameObjectManager->UpdateQEGameObjects();

        // UPDATE CULLING SCENE
        this->sessionManager->UpdateCullingScene();

        // UPDATE LIGHT SYSTEM
        this->lightManager->Update();

        // UPDATE ATMOSPHERE
        this->atmosphereSystem->UpdateSun();

        // UPDATE CAMERA DATA
        this->sessionManager->UpdateActiveCameraGPUData();

        // UPDATE DEBUG BUFFERS
        this->debugSystem->UpdateGraphicBuffers();

        OnBeginFrame();

        this->computeFrame();
        this->drawFrame();

        OnEndFrame();
    }
    vkDeviceWaitIdle(deviceModule->device);
}

void QEBaseApp::cleanUp()
{
    OnPreCleanup();

    this->shaderManager->Clean();

    //this->OmniShadowResources->cleanup();
    this->cleanUpSwapchain();
    //this->framebufferModule.cleanupShadowBuffer();
    this->swapchainModule->CleanScreenDataResources();

    this->sessionManager->CleanCameras();
    this->materialManager->CleanPipelines();
    this->computePipelineManager->CleanComputePipeline();
    this->computeNodeManager->Cleanup();
    this->lightManager->CleanShadowMapResources();

    this->atmosphereSystem->Cleanup();
    this->gameObjectManager->ReleaseAllGameObjects();
    this->particleSystemManager->Cleanup();
    this->sessionManager->CleanCullingResources();
    this->debugSystem->Cleanup();

    this->textureManager->Clean();

    this->shaderManager->CleanDescriptorSetLayouts();

    this->synchronizationModule.cleanup();
    this->commandPoolModule->cleanup();

    this->deviceModule->cleanup();

    if (enableValidationLayers)
    {
        this->layerExtensionModule.DestroyDebugUtilsMessengerEXT(vulkanInstance.getInstance(), nullptr);
    }

    this->windowSurface.cleanUp(vulkanInstance.getInstance());
    this->vulkanInstance.destroyInstance();

    glfwDestroyWindow(mainWindow->getWindow());

    glfwTerminate();

    this->cleanManagers();
}

void QEBaseApp::cleanUpSwapchain()
{
    antialiasingModule->cleanup();
    depthBufferModule->cleanup();
    framebufferModule.cleanup();

    vkFreeCommandBuffers(deviceModule->device, commandPoolModule->getCommandPool(), commandPoolModule->getNumCommandBuffers(), commandPoolModule->getCommandBuffers().data());

    renderPassModule->cleanup();

    //Limpiamos los VKPipelines, VkPipelineLayouts y shader del material
    graphicsPipelineManager->CleanGraphicsPipeline();
    shadowPipelineManager->CleanShadowPipelines();

    swapchainModule->cleanup();
}

void QEBaseApp::cleanManagers()
{
    //delete this->renderPassModule;
    this->renderPassModule = nullptr;

    this->graphicsPipelineManager->ResetInstance();
    this->graphicsPipelineManager = nullptr;

    this->shadowPipelineManager->ResetInstance();
    this->shadowPipelineManager = nullptr;

    this->computePipelineManager->ResetInstance();
    this->computePipelineManager = nullptr;

    this->atmosphereSystem->CleanLastResources();
    this->atmosphereSystem->ResetInstance();
    this->atmosphereSystem = nullptr;

    this->gameObjectManager->CleanLastResources();
    this->gameObjectManager->ResetInstance();
    this->gameObjectManager = nullptr;

    this->particleSystemManager->CleanLastResources();
    this->particleSystemManager->ResetInstance();
    this->particleSystemManager = nullptr;

    this->textureManager->CleanLastResources();
    this->textureManager->ResetInstance();
    this->textureManager = nullptr;

    this->keyboard_ptr->CleanLastResources();
    this->keyboard_ptr->ResetInstance();
    this->keyboard_ptr = nullptr;

    this->materialManager->CleanLastResources();
    this->materialManager->ResetInstance();
    this->materialManager = nullptr;

    this->shaderManager->CleanLastResources();
    this->shaderManager->ResetInstance();
    this->shaderManager = nullptr;

    this->lightManager->CleanLastResources();
    this->lightManager->ResetInstance();
    this->lightManager = nullptr;

    this->sessionManager->FreeCameraResources();

    this->antialiasingModule->CleanLastResources();
    this->antialiasingModule->ResetInstance();
    this->antialiasingModule = nullptr;

    this->depthBufferModule->CleanLastResources();
    this->depthBufferModule->ResetInstance();
    this->depthBufferModule = nullptr;

    this->shadowPipelineManager->ResetInstance();
    this->shadowPipelineManager = nullptr;

    this->physicsModule->ResetInstance();
    this->physicsModule = nullptr;

    this->computeNodeManager->CleanLastResources();
    this->computeNodeManager->ResetInstance();
    this->computeNodeManager = nullptr;

    this->commandPoolModule->CleanLastResources();
    this->commandPoolModule->ResetInstance();
    this->commandPoolModule = nullptr;

    this->swapchainModule->ResetInstance();
    this->swapchainModule = nullptr;

    this->queueModule->ResetInstance();
    this->queueModule = nullptr;

    this->deviceModule->ResetInstance();
    this->deviceModule = nullptr;
}

void QEBaseApp::computeFrame()
{
    if (this->isRender)
    {
        synchronizationModule.synchronizeWaitComputeFences();
        //Update uniformBuffer here -----> <-----

        // Update particles system
        this->particleSystemManager->UpdateParticleSystems();

        commandPoolModule->recordComputeCommandBuffer(commandPoolModule->getComputeCommandBuffer((uint32_t)synchronizationModule.GetCurrentFrame()));
        synchronizationModule.submitComputeCommandBuffer(commandPoolModule->getComputeCommandBuffer((uint32_t)synchronizationModule.GetCurrentFrame()));
    }
}

void QEBaseApp::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule->getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &swapchainModule->currentImage);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    //this->cameraEditor->UpdateUBOCamera();
    //this->lightManager->UpdateUBOLight();
    this->materialManager->UpdateUniforms();

    //vkDeviceWaitIdle(deviceModule->device);

    commandPoolModule->Render(&framebufferModule);

    synchronizationModule.submitCommandBuffer(commandPoolModule->getCommandBuffer((uint32_t)synchronizationModule.GetCurrentFrame()), this->isRender);

    result = synchronizationModule.presentSwapchain(swapchainModule->getSwapchain(), swapchainModule->currentImage);
    resizeSwapchain(result, ERROR_RESIZE::IMAGE_ERROR);
    this->isRender = true;
}

void QEBaseApp::resizeSwapchain(VkResult result, ERROR_RESIZE errorResize)
{
    if (errorResize == ERROR_RESIZE::SWAPCHAIN_ERROR)
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapchain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }
    else
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }
}

void QEBaseApp::recreateSwapchain()
{
    mainWindow->checkMinimize();

    vkDeviceWaitIdle(deviceModule->device);

    //Recreamos el swapchain
    cleanUpSwapchain();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow->getWindow());

    //Actualizamos el formato de la c�mara
    this->sessionManager->UpdateViewportSize();

    //Actualizamos la resoluci�n de la atmosfera
    this->atmosphereSystem->UpdateAtmopshereResolution();

    //Recreamos el antialiasing module
    antialiasingModule->createColorResources();
    //Recreamos el depth buffer module
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Recreamos el render pass
    renderPassModule->CreateRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);
    renderPassModule->CreateDirShadowRenderPass(VK_FORMAT_D32_SFLOAT);
    renderPassModule->CreateOmniShadowRenderPass(VK_FORMAT_R32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

    //Recreamos los graphics pipeline de los materiales
    graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->DefaultRenderPass);

    shaderManager->RecreateShaderGraphicsPipelines();
    //materialManager->RecreateMaterials(renderPassModule);

    //Recreamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->DefaultRenderPass);

    commandPoolModule->recreateCommandBuffers();
    commandPoolModule->Render(&framebufferModule);

    OnSwapchainRecreated();
}
