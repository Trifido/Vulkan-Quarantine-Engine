#include "QEBaseApp.h"

#include "SyncTool.h"

#include <BufferManageModule.h>
#include <filesystem>
#include <QEProjectManager.h>
#include <QEMeshRenderer.h>
#include <QECameraController.h>
#include <QESpringArmComponent.h>
#include <PlaneCollider.h>
#include <BoxCollider.h>
#include "PhysicsBody.h"
#include <CSMResources.h>
#include <OmniShadowResources.h>
#include <QERuntimeMode.h>
#include <CullingSceneManager.h>
#include <exception>
#include <stdexcept>

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

    std::exception_ptr failure;

    try
    {
        OnInitialize();
        mainLoop();
    }
    catch (...)
    {
        failure = std::current_exception();
    }

    try
    {
        OnShutdown();
    }
    catch (...)
    {
        if (!failure)
            failure = std::current_exception();
    }

    try
    {
        cleanUp();
    }
    catch (...)
    {
        if (!failure)
            failure = std::current_exception();
    }

    if (failure)
        std::rethrow_exception(failure);
}

void QEBaseApp::InitWindow()
{
    this->mainWindow = GUIWindow::getInstance();
    if (!this->mainWindow->init())
    {
        GUIWindow::ResetInstance();
        this->mainWindow = nullptr;
        glfwTerminate();
        throw std::runtime_error("Failed to initialize the application window");
    }
}

void QEBaseApp::initVulkan()
{
    vulkanInstance.debug_level = DEBUG_LEVEL::ONLY_ERROR;
    const VkResult instanceResult = vulkanInstance.createInstance();
    if (instanceResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance (VkResult " + std::to_string(instanceResult) + ")");
    }
    layerExtensionModule.setupDebugMessenger(vulkanInstance.getInstance(), vulkanInstance.debug_level);
    windowSurface.createSurface(vulkanInstance.getInstance(), mainWindow->getWindow());
    deviceModule->pickPhysicalDevice(vulkanInstance.getInstance(), windowSurface.getSurface());
    deviceModule->createLogicalDevice(windowSurface.getSurface(), *queueModule);

    //Inicializamos el CommandPool Module
    commandPoolModule = CommandPoolModule::getInstance();
    commandPoolModule->ClearColor = glm::vec3(0.0f);

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
    renderPassModule->CreateDirShadowRenderPass(CSMResources::GetSupportedShadowFormat(deviceModule));
    renderPassModule->CreateOmniShadowRenderPass(
        OmniShadowResources::GetSupportedColorFormat(deviceModule),
        OmniShadowResources::GetSupportedDepthFormat(deviceModule));

    renderPassModule->CreateViewportRenderPass(
        swapchainModule->swapChainImageFormat,
        depthBufferModule->findDepthFormat(),
        *antialiasingModule->msaaSamples);

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
    this->cameraContext = QECameraContext::getInstance();
    QERuntimeMode::getInstance()->SetGameplayEnabled(true);

    if (auto cullingSceneManager = CullingSceneManager::getInstance())
    {
        cullingSceneManager->DebugMode = false;
    }

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
    ConfigureEngineBindings();

    // Load Scene
    this->LoadCurrentScene();

    this->synchronizationModule.createSyncObjects();

    OnPostInitVulkan();
}

void QEBaseApp::loadScene(QEScene& scene)
{
    scene.DeserializeScene();
    physicsModule->SetGravity(scene.physicsGravity);

    OnBeforeSceneActivated();
    cameraContext->RegisterSceneCameras();
    cameraContext->ResolveActiveCamera();

    auto activeCamera = cameraContext->ActiveCamera();
    if (!activeCamera)
        throw std::runtime_error("No active camera resolved.");

    auto swapchainModule = SwapChainModule::getInstance();
    activeCamera->UpdateViewportSize(swapchainModule->swapChainExtent);
    activeCamera->UpdateCamera();

    lightManager->AddDirShadowMapShader(materialManager->GetCSMShader());
    lightManager->AddOmniShadowMapShader(materialManager->GetOmniShadowMappingShader());

    gameObjectManager->StartQEGameObjects();

    atmosphereSystem = AtmosphereSystem::getInstance();
    atmosphereSystem->LoadAtmosphereDto(scene.atmosphereDto);
}

bool QEBaseApp::LoadSceneFromPath(const std::filesystem::path& scenePath)
{
    if (!std::filesystem::exists(scenePath))
    {
        throw std::runtime_error("Scene path does not exist: " + scenePath.string());
    }

    vkDeviceWaitIdle(deviceModule->device);

    UnloadCurrentScene();

    QEScene newScene;
    if (!QEProjectManager::InitializeQEScene(newScene, scenePath))
    {
        throw std::runtime_error("Failed to initialize scene: " + scenePath.string());
    }

    this->scene = std::move(newScene);
    LoadCurrentScene();

    return true;
}

void QEBaseApp::UnloadCurrentScene()
{
    if (cameraContext)
    {
        cameraContext->ClearSceneCameras();
    }

    if (lightManager)
    {
        lightManager->ResetSceneState();
    }

    if (gameObjectManager)
    {
        gameObjectManager->ResetSceneState();
    }

    if (materialManager)
    {
        materialManager->ResetSceneState();
    }

    if (atmosphereSystem)
    {
        atmosphereSystem->ResetSceneState();
    }
}

void QEBaseApp::LoadCurrentScene()
{
    loadScene(this->scene);

    if (lightManager)
    {
        lightManager->InitializeShadowMaps();
    }

    if (gameObjectManager)
    {
        gameObjectManager->RegisterSceneLights();
    }

    if (atmosphereSystem)
    {
        atmosphereSystem->InitializeAtmosphereResources();
    }
}

void QEBaseApp::OnMainViewportResized(uint32_t width, uint32_t height)
{
    if (this->cameraContext)
    {
        this->cameraContext->UpdateGameCameraViewportSize(width, height);
    }
}

void QEBaseApp::mainLoop()
{
    while (!glfwWindowShouldClose(mainWindow->getWindow()))
    {
        glfwPollEvents();

        OnFrameStart();

        Timer::getInstance()->UpdateDeltaTime();
        uint32_t currentFrame = (uint32_t)synchronizationModule.GetCurrentFrame();

        this->debugSystem->ClearLines();

        // Start GameObjects
        this->gameObjectManager->StartQEGameObjects();

        // UI / editor interaction happens here
        OnBeginFrame();

        // PHYSICS
        int physicsSteps = Timer::getInstance()->ComputeFixedSteps();
        for (int i = 0; i < physicsSteps; ++i)
            physicsModule->ComputePhysics(Timer::getInstance()->FixedDelta);

        // UPDATE GameObjects after UI/input so editor controllers consume fresh ImGui state.
        this->gameObjectManager->UpdateQEGameObjects();

        // UPDATE CULLING SCENE
        if (auto cullingSceneManager = CullingSceneManager::getInstance())
        {
            cullingSceneManager->UpdateCullingScene();
        }

        OnEndFrame();

        // Ensure camera CPU data is up to date after editor interaction
        if (auto activeCamera = this->cameraContext->ActiveCamera())
        {
            activeCamera->UpdateCamera();
        }

        // UPDATE LIGHT SYSTEM AFTER editor edits
        this->lightManager->Update(currentFrame);

        // UPDATE ATMOSPHERE AFTER editor edits
        this->atmosphereSystem->UpdatePerFrame(currentFrame);

        // UPDATE DEBUG BUFFERS
        this->debugSystem->UpdateGraphicBuffers();

        this->drawFrame(currentFrame);
    }

    vkDeviceWaitIdle(deviceModule->device);
}

void QEBaseApp::cleanUp()
{
    if (this->deviceModule && this->deviceModule->device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(this->deviceModule->device);
    }

    OnPreCleanup();

    this->shaderManager->Clean();

    this->cleanUpSwapchain();
    this->swapchainModule->CleanScreenDataResources();

    this->cameraContext->ShutdownPersistentResources();
    this->lightManager->ShutdownPersistentResources();
    this->materialManager->CleanPipelines();
    this->computePipelineManager->CleanComputePipeline();
    this->computeNodeManager->Cleanup();

    this->atmosphereSystem->Cleanup();
    this->gameObjectManager->ReleaseAllGameObjects();
    this->particleSystemManager->Cleanup();
    if (auto cullingSceneManager = CullingSceneManager::getInstance())
    {
        cullingSceneManager->ResetSceneState();
    }
    this->debugSystem->Cleanup();

    this->lightManager->CleanShadowMapResources();
    this->textureManager->Clean();

    this->shaderManager->CleanDescriptorSetLayouts();

    this->synchronizationModule.cleanup();
    this->commandPoolModule->cleanup();

    // Destroy all services while their DeviceModule dependency is still alive.
    this->cleanManagers();

    this->deviceModule->cleanup();
    DeviceModule::ResetInstance();
    this->deviceModule = nullptr;

    QueueModule::ResetInstance();
    this->queueModule = nullptr;

    if (enableValidationLayers)
    {
        this->layerExtensionModule.DestroyDebugUtilsMessengerEXT(vulkanInstance.getInstance(), nullptr);
    }

    this->windowSurface.cleanUp(vulkanInstance.getInstance());
    this->vulkanInstance.destroyInstance();

    if (mainWindow && mainWindow->getWindow())
    {
        glfwDestroyWindow(mainWindow->getWindow());
        mainWindow->window = nullptr;
    }

    glfwTerminate();
    GUIWindow::ResetInstance();
    this->mainWindow = nullptr;
}

void QEBaseApp::cleanUpSwapchain(bool destroyRenderResources)
{
    antialiasingModule->cleanup();
    depthBufferModule->cleanup();
    framebufferModule.cleanup();

    commandPoolModule->freeGraphicsCommandBuffers();

    if (destroyRenderResources)
    {
        // Pipelines must be destroyed before the render passes they use.
        graphicsPipelineManager->CleanGraphicsPipeline();
        shadowPipelineManager->CleanShadowPipelines();
        renderPassModule->cleanup();
    }

    swapchainModule->cleanup();
}

void QEBaseApp::cleanManagers()
{
    GraphicsPipelineManager::ResetInstance();
    this->graphicsPipelineManager = nullptr;

    this->shadowPipelineManager->CleanLastResources();
    ShadowPipelineManager::ResetInstance();
    this->shadowPipelineManager = nullptr;

    ComputePipelineManager::ResetInstance();
    this->computePipelineManager = nullptr;

    RenderPassModule::ResetInstance();
    this->renderPassModule = nullptr;

    this->atmosphereSystem->CleanLastResources();
    AtmosphereSystem::ResetInstance();
    this->atmosphereSystem = nullptr;

    this->gameObjectManager->CleanLastResources();
    GameObjectManager::ResetInstance();
    this->gameObjectManager = nullptr;

    this->particleSystemManager->CleanLastResources();
    ParticleSystemManager::ResetInstance();
    this->particleSystemManager = nullptr;

    this->textureManager->CleanLastResources();
    TextureManager::ResetInstance();
    this->textureManager = nullptr;

    this->keyboard_ptr->CleanLastResources();
    KeyboardController::ResetInstance();
    this->keyboard_ptr = nullptr;

    this->materialManager->CleanLastResources();
    MaterialManager::ResetInstance();
    this->materialManager = nullptr;

    this->shaderManager->CleanLastResources();
    ShaderManager::ResetInstance();
    this->shaderManager = nullptr;

    this->lightManager->CleanLastResources();
    LightManager::ResetInstance();
    this->lightManager = nullptr;

    this->cameraContext->FreeCameraResources();
    this->cameraContext->ClearSceneCameras();
    QECameraContext::ResetInstance();
    this->cameraContext = nullptr;

    QEDebugSystem::ResetInstance();
    this->debugSystem = nullptr;

    CullingSceneManager::ResetInstance();

    this->antialiasingModule->CleanLastResources();
    AntiAliasingModule::ResetInstance();
    this->antialiasingModule = nullptr;

    this->depthBufferModule->CleanLastResources();
    DepthBufferModule::ResetInstance();
    this->depthBufferModule = nullptr;

    PhysicsModule::ResetInstance();
    this->physicsModule = nullptr;

    this->computeNodeManager->CleanLastResources();
    ComputeNodeManager::ResetInstance();
    this->computeNodeManager = nullptr;

    this->commandPoolModule->CleanLastResources();
    CommandPoolModule::ResetInstance();
    this->commandPoolModule = nullptr;

    SwapChainModule::ResetInstance();
    this->swapchainModule = nullptr;

    Timer::ResetInstance();
    QERuntimeMode::ResetInstance();
}

void QEBaseApp::computeFrame(uint32_t currentFrame)
{
    if (this->isRender)
    {
        synchronizationModule.synchronizeWaitComputeFences();

        this->cameraContext->UpdateActiveCameraGPUData(currentFrame);

        this->particleSystemManager->UpdateParticleSystems();

        commandPoolModule->recordComputeCommandBuffer(
            commandPoolModule->getComputeCommandBuffer(currentFrame));

        synchronizationModule.submitComputeCommandBuffer(
            commandPoolModule->getComputeCommandBuffer(currentFrame));
    }
}

void QEBaseApp::drawFrame(uint32_t currentFrame)
{
    synchronizationModule.synchronizeWaitFences();

    if (!mainWindow->HasUsableFramebufferSize())
    {
        recreateSwapchain();
        return;
    }

    VkResult result = vkAcquireNextImageKHR(
        deviceModule->device,
        swapchainModule->getSwapchain(),
        UINT64_MAX,
        synchronizationModule.getImageAvailableSemaphore(),
        VK_NULL_HANDLE,
        &swapchainModule->currentImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    const bool acquiredSuboptimalSwapchain = result == VK_SUBOPTIMAL_KHR;

    // Compute submission must happen only after a swapchain image was acquired.
    // Otherwise its binary semaphore could remain signaled without a graphics
    // submission consuming it.
    this->computeFrame(currentFrame);

    this->cameraContext->UpdateActiveCameraGPUData(currentFrame);
    this->materialManager->UpdateUniforms();

    commandPoolModule->Render(
        &framebufferModule,
        this->cameraContext->GetRenderTargetOverride(),
        [this](VkCommandBuffer& commandBuffer, uint32_t currentFrame)
        {
            RecordAdditionalScenePass(commandBuffer, currentFrame);
        },
        [this](VkCommandBuffer& commandBuffer, uint32_t currentFrame)
        {
            RecordAdditionalOverlayPass(commandBuffer, currentFrame);
        });

    // Reset only when this frame is guaranteed to submit work. If acquisition
    // returned OUT_OF_DATE, the fence remains signaled for the next frame.
    synchronizationModule.resetCurrentFrameFence();
    synchronizationModule.submitCommandBuffer(
        commandPoolModule->getCommandBuffer(currentFrame),
        this->isRender);

    result = synchronizationModule.presentSwapchain(
        swapchainModule->getSwapchain(),
        swapchainModule->currentImage);

    this->isRender = true;

    if (result != VK_SUCCESS &&
        result != VK_ERROR_OUT_OF_DATE_KHR &&
        result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    const bool framebufferResized = mainWindow->ConsumeFramebufferResized();
    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        acquiredSuboptimalSwapchain ||
        framebufferResized)
    {
        recreateSwapchain();
    }
}

void QEBaseApp::recreateSwapchain()
{
    if (!mainWindow->WaitForUsableFramebufferSize())
        return;

    vkDeviceWaitIdle(deviceModule->device);

    const VkFormat previousFormat = swapchainModule->swapChainImageFormat;
    const uint32_t previousMinImageCount = swapchainModule->getMinSwapChainImageCount();
    const uint32_t previousImageCount = swapchainModule->getNumSwapChainImages();

    OnBeforeSwapchainCleanup();
    cleanUpSwapchain(false);
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow->getWindow());

    const bool renderPassCompatibilityChanged =
        previousFormat != swapchainModule->swapChainImageFormat;
    const bool rendererConfigurationChanged =
        renderPassCompatibilityChanged ||
        previousMinImageCount != swapchainModule->getMinSwapChainImageCount() ||
        previousImageCount != swapchainModule->getNumSwapChainImages();

    if (rendererConfigurationChanged)
        OnBeforeSwapchainRendererRecreated();

    OnMainViewportResized(
        swapchainModule->swapChainExtent.width,
        swapchainModule->swapChainExtent.height);

    this->atmosphereSystem->UpdateAtmopshereResolution();

    antialiasingModule->createColorResources();
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    if (renderPassCompatibilityChanged)
    {
        graphicsPipelineManager->CleanGraphicsPipeline();
        shadowPipelineManager->CleanShadowPipelines();
        renderPassModule->cleanup();

        renderPassModule->CreateRenderPass(
            swapchainModule->swapChainImageFormat,
            depthBufferModule->findDepthFormat(),
            *antialiasingModule->msaaSamples);

        renderPassModule->CreateDirShadowRenderPass(CSMResources::GetSupportedShadowFormat(deviceModule));
        renderPassModule->CreateOmniShadowRenderPass(
            OmniShadowResources::GetSupportedColorFormat(deviceModule),
            OmniShadowResources::GetSupportedDepthFormat(deviceModule));
        renderPassModule->CreateViewportRenderPass(
            swapchainModule->swapChainImageFormat,
            depthBufferModule->findDepthFormat(),
            *antialiasingModule->msaaSamples);

        graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->DefaultRenderPass);
        shaderManager->RecreateShaderGraphicsPipelines();
    }

    framebufferModule.createFramebuffer(renderPassModule->DefaultRenderPass);

    commandPoolModule->recreateCommandBuffers();

    OnSwapchainRecreated();
    mainWindow->ConsumeFramebufferResized();
}

