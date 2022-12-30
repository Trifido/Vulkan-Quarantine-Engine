#include "App.h"

#include "SyncTool.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "../Editor/Grid.h"


App::App()
{
    this->deltaTime = this->lastFrame = 0;

    this->keyboard_ptr = KeyboardController::getInstance();
    this->commandPoolModule = CommandPoolModule::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->deviceModule = DeviceModule::getInstance();

    this->physicsModule = PhysicsModule::getInstance();
    this->editorManager = EditorObjectManager::getInstance();
    this->animationManager = AnimationManager::getInstance();

    commandPoolModule->ClearColor = glm::vec3(0.1f);
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
    this->deltaTime = currentFrame - lastFrame;
    this->lastFrame = currentFrame;
}

void App::initWindow()
{
    this->mainWindow.init();
    this->cameraEditor = CameraEditor::getInstance();
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

    //Añadimos requisitos para los geometryComponent
    BufferManageModule::commandPool = this->commandPoolModule->getCommandPool();
    BufferManageModule::graphicsQueue = this->queueModule->graphicsQueue;
    GeometryComponent::deviceModule_ptr = this->deviceModule;
    TextureManagerModule::queueModule = this->queueModule;
    CustomTexture::commandPool = commandPoolModule->getCommandPool();
    DescriptorModule::deviceModule = this->deviceModule;
    DescriptorModule::NumSwapchainImages = this->swapchainModule->getNumSwapChainImages();

    // INIT ------------------------- Mesh & Material -------------------------------
    this->shaderManager = ShaderManager::getInstance();
    this->textureManager = TextureManager::getInstance();
    this->lightManager = LightManager::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->materialManager->InitializeMaterialManager(renderPassModule->renderPass, graphicsPipelineModule);
    this->gameObjectManager = GameObjectManager::getInstance();

    // Inicializamos los componentes del editor
    std::shared_ptr<Grid> grid_ptr = std::make_shared<Grid>(Grid(graphicsPipelineModule, renderPassModule->renderPass));
    this->editorManager->AddEditorObject(grid_ptr, "editor:grid");


    std::shared_ptr<GameObject> model = std::make_shared<GameObject>(GameObject("../../resources/models/Raptoid/scene.gltf"));
    //std::shared_ptr<GameObject> model = std::make_shared<GameObject>(GameObject("../../resources/models/vampire/Capoeira.dae"));
    //std::shared_ptr<GameObject> model = std::make_shared<GameObject>(GameObject("../../resources/models/CharacterRunning/CharacterRunning.gltf"));

    if (model->IsValid())
    {
        model->transform->SetScale(glm::vec3(0.05f));
        model->transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    this->gameObjectManager->AddGameObject(model, "model");


/*
    //Creamos la textura
    textureManager->AddTexture("diffuse_brick", CustomTexture(TEXTURE_WALL_PATH, TEXTURE_TYPE::DIFFUSE_TYPE));
    textureManager->AddTexture("normal_brick", CustomTexture(TEXTURE_WALL_NORMAL_PATH, TEXTURE_TYPE::NORMAL_TYPE));
    textureManager->AddTexture("test", CustomTexture(TEXTURE_TEST_PATH, TEXTURE_TYPE::DIFFUSE_TYPE));

    std::shared_ptr<GameObject> cube = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::CUBE_TYPE));
    cube->transform->SetPosition(glm::vec3(0.0f, 20.0f, 0.0f));
    cube->transform->SetOrientation(glm::vec3(0.0f, 0.0f, 65.0f));

    std::shared_ptr<GameObject> plano = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::PLANE_TYPE));
    plano->transform->SetOrientation(glm::vec3(0.0f, 0.0f, 45.0f));
    plano->transform->SetPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    plano->transform->SetScale(glm::vec3(5.0f, 1.0f, 5.0f));

    std::shared_ptr<GameObject> floor = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::PLANE_TYPE));
    floor->transform->SetPosition(glm::vec3(0.0f, -0.10f, 0.0f));
    floor->transform->SetScale(glm::vec3(50.0f, 1.0f, 50.0f));

    this->gameObjectManager->AddGameObject(cube, "cube");
    this->gameObjectManager->AddGameObject(plano, "planoInclinado");
    this->gameObjectManager->AddGameObject(floor, "floor");

    models.push_back(std::make_shared<GameObject>(GameObject(MODEL_CRYSIS_PATH)));
    models.at(0)->transform->SetScale(glm::vec3(0.1f));
    models.at(0)->transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
   
    //Creamos el shader module para el material
    std::shared_ptr<ShaderModule> shader_ptr = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv"));
    shader_ptr->createShaderBindings();
    this->shaderManager->AddShader("shader", shader_ptr);

    //Creamos el material
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(Material(this->shaderManager->GetShader("shader"), renderPassModule->renderPass));
    mat_ptr->AddNullTexture(textureManager->GetTexture("NULL"));
    mat_ptr->AddTexture(textureManager->GetTexture("diffuse_brick"));
    mat_ptr->AddTexture(textureManager->GetTexture("normal_brick"));
    mat_ptr->AddPipeline(graphicsPipelineModule);
    materialManager->AddMaterial("mat", mat_ptr);

    std::shared_ptr<Material> mat_ptr2 = std::make_shared<Material>(Material(this->shaderManager->GetShader("shader"), renderPassModule->renderPass));
    mat_ptr2->AddNullTexture(textureManager->GetTexture("NULL"));
    mat_ptr2->AddTexture(textureManager->GetTexture("test"));
    mat_ptr2->AddPipeline(graphicsPipelineModule);
    materialManager->AddMaterial("mat2", mat_ptr2);


    //Linkamos el material al gameobject
    cube->addMaterial(materialManager->GetMaterial("mat"));
    plano->addMaterial(materialManager->GetMaterial("mat"));
    floor->addMaterial(materialManager->GetMaterial("mat2"));
*/
    // END -------------------------- Mesh & Material -------------------------------

    // INIT ------------------------- Lights ----------------------------------------
    //this->lightManager->CreateLight(LightType::POINT_LIGHT, "PointLight0");
    //this->lightManager->GetLight("PointLight0")->transform->SetPosition(glm::vec3(0.0f, 0.0f, -3.0f));
    //this->lightManager->GetLight("PointLight0")->diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
    //this->lightManager->GetLight("PointLight0")->specular = glm::vec3(0.8f);
    //this->lightManager->GetLight("PointLight0")->linear = 0.09f;
    //this->lightManager->GetLight("PointLight0")->quadratic = 0.032f;

    //this->lightManager->CreateLight(LightType::POINT_LIGHT, "PointLight1");
    //this->lightManager->GetLight("PointLight1")->transform->SetPosition(glm::vec3(0.5f, 1.0f, 0.3f));
    //this->lightManager->GetLight("PointLight1")->diffuse = glm::vec3(0.1f);
    //this->lightManager->GetLight("PointLight1")->specular = glm::vec3(0.5f);
    //this->lightManager->GetLight("PointLight1")->linear = 0.09f;
    //this->lightManager->GetLight("PointLight1")->quadratic = 0.032f;

    this->lightManager->CreateLight(LightType::DIRECTIONAL_LIGHT, "DirectionalLight0");
    //this->lightManager->GetLight("DirectionalLight0")->transform->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    this->lightManager->GetLight("DirectionalLight0")->transform->SetOrientation(glm::vec3(2.0f, 8.0f, -1.0f));
    this->lightManager->GetLight("DirectionalLight0")->diffuse = glm::vec3(0.1f);
    this->lightManager->GetLight("DirectionalLight0")->specular = glm::vec3(0.1f);
    this->lightManager->GetLight("DirectionalLight0")->constant = 1.0f;
    this->lightManager->GetLight("DirectionalLight0")->linear = 0.09f;
    this->lightManager->GetLight("DirectionalLight0")->quadratic = 0.032f;

    this->lightManager->UpdateUniform();
    // END -------------------------- Lights ----------------------------------------

    /**/

    // Initialize Physics
    /*
    std::shared_ptr<PhysicBody> rigidBody = std::make_shared<PhysicBody>(PhysicBody(PhysicBodyType::RIGID_BODY));
    rigidBody->Mass = 0.001f;
    cube->addPhysicBody(rigidBody);
    std::shared_ptr<BoxCollider> boxCollider = std::make_shared<BoxCollider>();
    cube->addCollider(boxCollider);

    std::shared_ptr<PhysicBody> staticBody = std::make_shared<PhysicBody>();
    plano->addPhysicBody(staticBody);
    std::shared_ptr<PlaneCollider> planeCollider = std::make_shared<PlaneCollider>();
    planeCollider->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    plano->addCollider(planeCollider);

    std::shared_ptr<PhysicBody> staticBody2 = std::make_shared<PhysicBody>();
    floor->addPhysicBody(staticBody2);
    std::shared_ptr<PlaneCollider> planeCollider2 = std::make_shared<PlaneCollider>();
    planeCollider2->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    floor->addCollider(planeCollider2);
    */

    // Initialize Managers
    this->animationManager->InitializeAnimations();
    //this->animationManager->UpdateAnimations(0.0f);
    this->gameObjectManager->InitializePhysics();
    this->materialManager->InitializeMaterials();

    this->commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass);
    this->synchronizationModule.createSyncObjects(swapchainModule->getNumSwapChainImages());

    init_imgui();
}

void App::mainLoop()
{
    bool changeAnimation = true;

    while (!glfwWindowShouldClose(mainWindow.getWindow())) {
        glfwPollEvents();
        computeDeltaTime();

        //PHYSIC SYSTEM
        this->physicsModule->ComputePhysics((float)this->deltaTime);

        // Update transforms
        this->gameObjectManager->UpdatePhysicTransforms();

        //ANIMATION SYSTEM
        this->animationManager->UpdateAnimations((float)this->deltaTime);

        // INPUT EVENTS
        this->keyboard_ptr->ReadKeyboardEvents();
        this->cameraEditor->CameraController((float)deltaTime);

        if (ImGui::IsKeyDown('j') || ImGui::IsKeyDown('J'))
        {
            glm::vec3 newPos = this->lightManager->GetLight("DirectionalLight0")->transform->Rotation;
            newPos.x += 0.01f;
            this->lightManager->GetLight("DirectionalLight0")->transform->SetOrientation(newPos);
            this->lightManager->UpdateUniform();
        }
        if (ImGui::IsKeyDown('l') || ImGui::IsKeyDown('L'))
        {
            glm::vec3 newPos = this->lightManager->GetLight("DirectionalLight0")->transform->Rotation;
            newPos.x -= 0.01f;
            this->lightManager->GetLight("DirectionalLight0")->transform->SetOrientation(newPos);
            this->lightManager->UpdateUniform();
        }
        if (ImGui::IsKeyDown('I'))
        {
            glm::vec3 newPos = this->lightManager->GetLight("DirectionalLight0")->transform->Rotation;
            newPos.y += 0.01f;
            this->lightManager->GetLight("DirectionalLight0")->transform->SetOrientation(newPos);
            this->lightManager->UpdateUniform();
        }
        if (ImGui::IsKeyDown('K'))
        {
            glm::vec3 newPos = this->lightManager->GetLight("DirectionalLight0")->transform->Rotation;
            newPos.y -= 0.01f;
            this->lightManager->GetLight("DirectionalLight0")->transform->SetOrientation(newPos);
            this->lightManager->UpdateUniform();
        }

        if (ImGui::IsKeyDown('1'))
        {
            if (changeAnimation)
            {
                this->animationManager->ChangeAnimation();
                changeAnimation = false;
            }
        }
        if (ImGui::IsKeyReleased('1'))
        {
            changeAnimation = true;
        }

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
    shaderManager->Clean();

    cleanUpSwapchain();

    this->gameObjectManager->Cleanup();
    this->editorManager->Cleanup();

    textureManager->Clean();

    materialManager->CleanDescriptors();

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

    delete physicsModule;
}

void App::cleanUpSwapchain()
{
    antialiasingModule->cleanup();
    depthBufferModule->cleanup();
    framebufferModule.cleanup();

    vkFreeCommandBuffers(deviceModule->device, commandPoolModule->getCommandPool(), commandPoolModule->getNumCommandBuffers(), commandPoolModule->getCommandBuffers().data());

    renderPassModule->cleanup();

    //Limpiamos los VKPipelines, VkPipelineLayouts y shader del material
    materialManager->CleanPipelines();

    swapchainModule->cleanup();
}

void App::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule->getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    synchronizationModule.synchronizeCurrentFrame(imageIndex);

    materialManager->UpdateUniforms(imageIndex);

    vkDeviceWaitIdle(deviceModule->device);

    commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass);

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

void App::recreateSwapchain()
{
    mainWindow.checkMinimize();

    vkDeviceWaitIdle(deviceModule->device);

    //Recreamos el swapchain
    cleanUpSwapchain();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());

    //Actualizamos el formato de la cámara
    this->cameraEditor->UpdateSize(swapchainModule->swapChainExtent);

    //Recreamos el antialiasing module
    antialiasingModule->createColorResources();
    //Recreamos el depth buffer module
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Recreamos el render pass
    renderPassModule->createRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);

    //Recreamos los graphics pipeline de los materiales
    materialManager->RecreateMaterials(renderPassModule);

    //Recreamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->renderPass);

    commandPoolModule->createCommandBuffers();
    commandPoolModule->Render(framebufferModule.swapChainFramebuffers, renderPassModule->renderPass);
}
