#include "App.h"

#include "SyncTool.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

App::App()
{
    deltaTime = lastFrame = 0;

    keyboard_ptr = KeyboardController::getInstance();
    commandPoolModule = CommandPoolModule::getInstance();
    queueModule = QueueModule::getInstance();
    deviceModule = DeviceModule::getInstance();
}

void App::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void App::computeDeltaTime()
{
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}

void App::initWindow()
{
    mainWindow.init();
    camera_ptr = std::make_shared<Camera>(Camera(1280, 720));
}

void App::init_imgui()
{
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    vkCreateDescriptorPool(deviceModule->device, &pool_info, nullptr, &imguiPool);


    // 2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    //this initializes imgui for GLFW
    ImGui_ImplGlfw_InitForVulkan(mainWindow.window, true);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkanInstance.getInstance();
    init_info.PhysicalDevice = deviceModule->physicalDevice;
    init_info.Device = deviceModule->device;
    init_info.Queue = queueModule->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = *deviceModule->getMsaaSamples();//VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, renderPassModule->renderPass);

    //execute a gpu command to upload imgui font textures
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, commandPoolModule->getCommandPool());
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, commandPoolModule->getCommandPool(), commandBuffer);

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void App::addWindow(GLFWwindow& window)
{
    mainWindow.window = &window;
}

void App::initVulkan()
{
    vulkanInstance.debug_level = DEBUG_LEVEL::ONLY_ERROR;
    vulkanInstance.createInstance();
    layerExtensionModule.setupDebugMessenger(vulkanInstance.getInstance(), vulkanInstance.debug_level);
    windowSurface.createSurface(vulkanInstance.getInstance(), mainWindow.getWindow());
    deviceModule->pickPhysicalDevice(vulkanInstance.getInstance(), windowSurface.getSurface());
    deviceModule->createLogicalDevice(windowSurface.getSurface(), *queueModule);

    //Inicializamos el Swapchain Module
    swapchainModule = SwapChainModule::getInstance();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());

    //Creamos el Command pool module y los Command buffers
    commandPoolModule->createCommandPool(windowSurface.getSurface());
    commandPoolModule->createCommandBuffers();

    //Creamos el antialiasing module
    antialiasingModule = AntiAliasingModule::getInstance();
    antialiasingModule->createColorResources();

    //Creamos el depth buffer module
    depthBufferModule = DepthBufferModule::getInstance();
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Creamos el graphics pipeline Module
    graphicsPipelineModule = std::make_shared<GraphicsPipelineModule>();

    //Creamos el Render Pass
    renderPassModule = new RenderPassModule();
    renderPassModule->createRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);

    //Creamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->renderPass);

    // INIT ------------------------- Mesh & Material -------------------------------

    //Creamos la textura
    albedo = std::make_shared<Texture>();
    albedo->createTextureImage(TEXTURE_PATH, commandPoolModule->getCommandPool());
    
    //Creamos el descriptor para el shader y lo inicializamos
    descriptorModule = std::make_shared<DescriptorModule>(DescriptorModule(*deviceModule));
    descriptorModule->init(swapchainModule->getNumSwapChainImages(), *albedo);

    models.push_back(std::make_shared<GameObject>(GameObject(MODEL_PATH, commandPoolModule->getCommandPool(), descriptorModule)));    //Esto hay que cambiarlo

    //Creamos el shader module
    shaderModule = std::make_shared<ShaderModule>(ShaderModule());
    shaderModule->createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv", models.at(0)->mesh);

    //Creamos el material
    _materials["mat"] = std::make_shared<Material>(Material(shaderModule, descriptorModule));
    _materials["mat"]->addAlbedo(albedo);
    _materials["mat"]->initPipelineMaterial(graphicsPipelineModule, renderPassModule->renderPass);

    //Linkamos el material al gameobject
    models.at(0)->addMaterial(_materials["mat"]);
    // END -------------------------- Mesh & Material -------------------------------


    commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass, models);

    synchronizationModule.createSyncObjects(swapchainModule->getNumSwapChainImages());

    init_imgui();
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(mainWindow.getWindow())) {
        glfwPollEvents();
        computeDeltaTime();
        keyboard_ptr->ReadKeyboardEvents();
        camera_ptr->CameraController((float)deltaTime);

        {
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Render();

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
            drawFrame();
        }
        /*
        {
            //imgui new frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            //imgui commands
            //ImGui::ShowDemoWindow(&show_demo_window);

            mainWindow.renderMainWindow();

            // Game Rendering Window
            ImGui::Begin("GameWindow");
            {
                // Using a Child allow to fill all the space of the window.
                // It also alows customization
                ImGui::BeginChild("GameRender");
                // Get the size of the child (i.e. the whole draw size of the windows).
                ImVec2 wsize = ImGui::GetWindowSize();
                // Because I use the texture from OpenGL, I need to invert the V from the UV.
                auto tex = model->descriptorModule->getDescriptorSet()[1];
                ImGui::Image((ImTextureID)tex, wsize, ImVec2(0, 1), ImVec2(1, 0));
                ImGui::EndChild();
            }
            ImGui::End();

            ImGui::Render();
            drawFrame();

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }*/
    }
    vkDeviceWaitIdle(deviceModule->device);

    vkDestroyDescriptorPool(deviceModule->device, imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::cleanUp()
{
    cleanUpSwapchain();

    for (uint32_t i = 0; i < models.size(); i++)
    {
        models.at(i)->cleanup();
    }

    for (auto& it : _materials)
    {
        it.second->cleanupTextures();
    }

    descriptorModule->cleanup();

    synchronizationModule.cleanup();
    commandPoolModule->cleanup();

    deviceModule->cleanup();

    if (enableValidationLayers) {
        layerExtensionModule.DestroyDebugUtilsMessengerEXT(vulkanInstance.getInstance(), nullptr);
    }
    windowSurface.cleanUp(vulkanInstance.getInstance());
    vulkanInstance.destroyInstance();

    glfwDestroyWindow(mainWindow.getWindow());

    glfwTerminate();
}

void App::cleanUpSwapchain()
{
    antialiasingModule->cleanup();
    depthBufferModule->cleanup();
    framebufferModule.cleanup();

    vkFreeCommandBuffers(deviceModule->device, commandPoolModule->getCommandPool(), commandPoolModule->getNumCommandBuffers(), commandPoolModule->getCommandBuffers().data());

    renderPassModule->cleanup();
    //Limpiamos los VKPipelines, VkPipelineLayouts y shader del material
    for (auto& it : _materials)
    {
        it.second->cleanup();
    }

    swapchainModule->cleanup();

    descriptorModule->cleanupDescriptorBuffer();
    descriptorModule->cleanupDescriptorPool();
}

void App::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule->getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    synchronizationModule.synchronizeCurrentFrame(imageIndex);

    for (uint32_t i = 0; i < models.size(); i++)
    {
        descriptorModule->updateUniformBuffer(/*imageIndex,*/ swapchainModule->swapChainExtent, models.at(i)->transform, i);
    }

    vkDeviceWaitIdle(deviceModule->device);

    commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass, models);

    synchronizationModule.submitCommandBuffer(commandPoolModule->getCommandBuffer(imageIndex));

    result = synchronizationModule.presentSwapchain(swapchainModule->getSwapchain(), imageIndex);
    resizeSwapchain(result, ERROR_RESIZE::IMAGE_ERROR);
}

void App::resizeSwapchain(VkResult result, ERROR_RESIZE errorResize)
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
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||  framebufferResized)
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

void App::recreateSwapchain()
{
    mainWindow.checkMinimize();

    vkDeviceWaitIdle(deviceModule->device);

    //Recreamos el swapchain
    cleanUpSwapchain();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());

    shaderModule->createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv", models.at(0)->mesh);

    //Recreamos el antialiasing module
    antialiasingModule->createColorResources();
    //Recreamos el depth buffer module
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Recreamos el render pass
    renderPassModule->createRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);

    //Recreamos los graphics pipeline de los materiales
    for (auto& it : _materials)
    {
        it.second->recreatePipelineMaterial(renderPassModule->renderPass);
    }

    //Recreamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->renderPass);

    descriptorModule->recreateUniformBuffer(swapchainModule->getNumSwapChainImages());

    commandPoolModule->createCommandBuffers();

    commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass, models);
}
