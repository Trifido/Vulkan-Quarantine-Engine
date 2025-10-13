#include "App.h"

#include "SyncTool.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <BufferManageModule.h>
#include <filesystem>
#include "../Editor/Grid.h"
#include <QEProjectManager.h>
#include <QEMeshRenderer.h>

App::App()  
{ 
    this->timer = Timer::getInstance();  
    this->keyboard_ptr = KeyboardController::getInstance();  
    this->queueModule = QueueModule::getInstance();  
    this->deviceModule = DeviceModule::getInstance();  

    this->physicsModule = PhysicsModule::getInstance();  
    this->graphicsPipelineManager = GraphicsPipelineManager::getInstance();  
    this->shadowPipelineManager = ShadowPipelineManager::getInstance();  
    this->computePipelineManager = ComputePipelineManager::getInstance();  
}

App::~App()
{

}

void App::run(QEScene scene, bool isEditorMode)
{
    this->scene = scene;
    this->isRunEditor = isEditorMode;

    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void App::initWindow()
{
    this->mainWindow = GUIWindow::getInstance();
    this->mainWindow->init();
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
    ImGui_ImplGlfw_InitForVulkan(mainWindow->window, true);

    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vulkanInstance.getInstance();
    init_info.PhysicalDevice = deviceModule->physicalDevice;
    init_info.Device = deviceModule->device;
    init_info.Queue = queueModule->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.Subpass = 0;                      // normalmente el primero
    init_info.RenderPass = *this->renderPassModule->ImGuiRenderPass;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = *deviceModule->getMsaaSamples();//VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    //execute a gpu command to upload imgui font textures
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, commandPoolModule->getCommandPool());
    ImGui_ImplVulkan_CreateFontsTexture();
    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, commandPoolModule->getCommandPool(), commandBuffer);
}

void App::initVulkan()
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
    renderPassModule->CreateImGuiRenderPass(swapchainModule->swapChainImageFormat, *antialiasingModule->msaaSamples);
    renderPassModule->CreateRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);
    renderPassModule->CreateDirShadowRenderPass(VK_FORMAT_D32_SFLOAT);
    renderPassModule->CreateOmniShadowRenderPass(VK_FORMAT_R32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

    //Registramos el default render pass
    this->graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->DefaultRenderPass);

    //Creamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->DefaultRenderPass);

    //Añadimos requisitos para los geometryComponent
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

    // INIT ------------------------- Mesh & Material -------------------------------
    this->sessionManager = QESessionManager::getInstance();
    this->sessionManager->SetEditorMode(this->isRunEditor);

    this->shaderManager = ShaderManager::getInstance();
    this->textureManager = TextureManager::getInstance();
    this->lightManager = LightManager::getInstance();
    this->materialManager = MaterialManager::getInstance();
    this->materialManager->InitializeMaterialManager();
    this->gameObjectManager = GameObjectManager::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();
    this->computeNodeManager->InitializeComputeResources();
    this->particleSystemManager = ParticleSystemManager::getInstance();

    // Import meshes
    //QEProjectManager::ImportMeshFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Raptoid/scene.gltf");
    //QEProjectManager::ImportMeshFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Golem/scene.gltf");
    //QEProjectManager::ImportMeshFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Character/Idle_Character.glb");

    // Import animations
    //QEProjectManager::ImportAnimationFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Character/Idle_Character.glb",
    //    std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Character/Animations"));

    //QEProjectManager::ImportAnimationFile("C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/resources/models/Character/Hurricane.glb",
    //    std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Character/Animations"));

    // Load Scene
    this->loadScene(this->scene);

    this->sessionManager->SetupEditor();

    auto absPath = std::filesystem::absolute("../../resources/models").generic_string();

    //const std::string absolute_path = absPath + "/cyber_warrior/scene.gltf";
    //const std::string absolute_path = absPath + "/drone/mech_drone.glb";
    //const std::string absolute_path = absPath + "/newell_teaset/teapot.obj";
    //const std::string absolute_path = absPath + "/Raptoid/scene.gltf";

    auto characterPath = std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Character/Meshes/Idle_Character.gltf").generic_string();
    //auto characterPath = std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Golem/Meshes/scene.gltf").generic_string();
    //auto characterPath = std::filesystem::absolute("../../QEProjects/QEExample/QEAssets/QEModels/Raptoid/Meshes/scene.gltf").generic_string();

    // THIRD PERSON CAMERA
    //std::shared_ptr<QEGameObject> cameraObject = std::make_shared<QEGameObject>();
    //cameraObject->AddComponent(std::make_shared<QECamera>());
    //this->gameObjectManager->AddGameObject(cameraObject, "cameraObject");
    //auto cameraComponent = cameraObject->GetComponent<QECamera>();
    //cameraComponent->cameraFront = glm::vec3(0.0134427, -0.270601, 0.962598);
    //cameraComponent->cameraPos = glm::vec3(-0.715416, 1.89489, -2.56881);

    // CHARACTER CONTROLLER
    /**/
    std::shared_ptr<QEGeometryComponent> geometryComponent = make_shared<QEGeometryComponent>(std::make_unique<MeshGenerator>(characterPath));

    std::shared_ptr<QEGameObject> character = std::make_shared<QEGameObject>();
    character->AddComponent<QEGeometryComponent>(geometryComponent);
    character->AddComponent<QEMeshRenderer>(std::make_shared<QEMeshRenderer>());
    character->AddComponent<PhysicsBody>(std::make_shared<PhysicsBody>(PhysicBodyType::RIGID_BODY));
    character->AddComponent<QECollider>(std::make_shared<CapsuleCollider>());

    auto characterPBody = character->GetComponent<PhysicsBody>();
    characterPBody->Mass = 70.0f;
    characterPBody->CollisionGroup = CollisionFlag::COL_PLAYER;
    characterPBody->CollisionMask = CollisionFlag::COL_SCENE;

    character->AddComponent<QECharacterController>(std::make_shared<QECharacterController>());
    auto characterController = character->GetComponent<QECharacterController>();
    characterController->SetJumpForce(9.0f);
    characterController->AddGLFWWindow(this->mainWindow->getWindow());
    this->gameObjectManager->AddGameObject(character, "character");
    /**/

    //model->_Transform->SetPosition(glm::vec3(-3.5f, 1.3f, -2.0f));
    //model->transform->SetOrientation(glm::vec3(-90.0f, 180.0f, 0.0f));
    //model->_Transform->SetScale(glm::vec3(0.01f));
    //model->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.7f, 0.2f));
    //model->_Material->materialData.SetMaterialField("Specular", glm::vec3(0.5f, 0.5f, 0.5f));
    //model->_Material->materialData.SetMaterialField("Ambient", glm::vec3(0.2f));
    //this->gameObjectManager->AddGameObject(model, "modelRaptoid");

    /**/

    auto defaultMat = this->materialManager->GetMaterial("defaultPrimitiveMat");
    auto floorMatInstance = defaultMat->CreateMaterialInstance();
    this->materialManager->AddMaterial(floorMatInstance);
    
    std::shared_ptr<QEGameObject> floor = std::make_shared<QEGameObject>();
    std::shared_ptr<QEGeometryComponent> geometryFloorComponent = make_shared<QEGeometryComponent>(std::make_unique<FloorGenerator>());
    floor->AddComponent<QEGeometryComponent>(geometryFloorComponent);
    floor->AddComponent<QEMeshRenderer>(std::make_shared<QEMeshRenderer>());
    floor->AddComponent<QEMaterial>(floorMatInstance);
    
    auto floorTransform = floor->GetComponent<QETransform>();
    floorTransform->SetLocalPosition(glm::vec3(0.0f, -0.01f, 0.0f));
    floorTransform->SetLocalScale(glm::vec3(10.0f, 1.0f, 10.0f));

    floorMatInstance->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.2f, 0.7f));
    floorMatInstance->materialData.SetMaterialField("Specular", glm::vec3(0.0f, 0.0f, 0.0f));
    floorMatInstance->materialData.SetMaterialField("Ambient", glm::vec3(0.2f));
    
    floor->AddComponent<PhysicsBody>(std::make_shared<PhysicsBody>());
    auto floorPBody = floor->GetComponent<PhysicsBody>();
    floorPBody->CollisionGroup = CollisionFlag::COL_SCENE;
    floorPBody->CollisionMask = CollisionFlag::COL_PLAYER;
    floor->AddComponent<QECollider>(std::make_shared<PlaneCollider>());

    auto floorCollider = floor->GetComponent<QECollider>();
    std::static_pointer_cast<PlaneCollider>(floorCollider)->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    this->gameObjectManager->AddGameObject(floor, "floor");  

    auto rampMatInstance = defaultMat->CreateMaterialInstance();
    this->materialManager->AddMaterial(rampMatInstance);
    std::shared_ptr<QEGameObject> ramp = std::make_shared<QEGameObject>();
    std::shared_ptr<QEGeometryComponent> geometryCubeComponent = make_shared<QEGeometryComponent>(std::make_unique<CubeGenerator>());
    ramp->AddComponent<QEGeometryComponent>(geometryCubeComponent);
    ramp->AddComponent<QEMeshRenderer>(std::make_shared<QEMeshRenderer>());
    ramp->AddComponent<QEMaterial>(rampMatInstance);
    auto rampTransform = ramp->GetComponent<QETransform>();
    rampTransform->SetLocalPosition(glm::vec3(0.0f, -0.5f, -10.0f));
    rampTransform->SetLocalEulerDegrees(glm::vec3(70.0f, 0.0f, 0.0f));
    rampTransform->SetLocalScale(glm::vec3(3.0f, 10.0f, 3.0f));

    rampMatInstance->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.7f, 0.2f));

    ramp->AddComponent<PhysicsBody>(std::make_shared<PhysicsBody>());
    auto rampPBody = ramp->GetComponent<PhysicsBody>();
    rampPBody->CollisionGroup = CollisionFlag::COL_SCENE;
    rampPBody->CollisionMask = CollisionFlag::COL_PLAYER;

    ramp->AddComponent<BoxCollider>(std::make_shared<BoxCollider>());
    this->gameObjectManager->AddGameObject(ramp, "ramp");
    
    auto wallMatInstance = defaultMat->CreateMaterialInstance();
    this->materialManager->AddMaterial(wallMatInstance);
    std::shared_ptr<QEGameObject> wall = std::make_shared<QEGameObject>();
    std::shared_ptr<QEGeometryComponent> geometryWallComponent = make_shared<QEGeometryComponent>(std::make_unique<CubeGenerator>());
    wall->AddComponent<QEGeometryComponent>(geometryWallComponent);
    wall->AddComponent<QEMeshRenderer>(std::make_shared<QEMeshRenderer>());
    wall->AddComponent<QEMaterial>(wallMatInstance);
    auto wallTransform = wall->GetComponent<QETransform>();
    wallTransform->SetLocalPosition(glm::vec3(3.0f, 0.5f, 0.0f));
    wallTransform->SetLocalEulerDegrees(glm::vec3(0.0f, 0.0f, 0.0f));
    wallTransform->SetLocalScale(glm::vec3(4.0f, 0.5f, 4.0f));

    wallMatInstance->materialData.SetMaterialField("Diffuse", glm::vec3(0.8f, 0.2f, 0.2f));

    wall->AddComponent<PhysicsBody>(std::make_shared<PhysicsBody>());
    auto wallPBody = wall->GetComponent<PhysicsBody>();
    wallPBody->CollisionGroup = CollisionFlag::COL_SCENE;
    wallPBody->CollisionMask = CollisionFlag::COL_PLAYER;

    wall->AddComponent<QECollider>(std::make_shared<BoxCollider>());
    //this->gameObjectManager->AddGameObject(wall, "wall");
    /**/

    // END -------------------------- Mesh & Material -------------------------------

    // INIT ------------------------- Lights ----------------------------------------
    // POINT LIGHTS
    {
        //std::shared_ptr<QEGameObject> pointLight = std::make_shared<QEGameObject>();
        //auto pointLight1 = this->lightManager->CreateLight(LightType::POINT_LIGHT, "LuzPuntual");
        //pointLight->AddComponent(pointLight1);
        //auto pointLightTransform = pointLight->GetComponent<Transform>();
        //pointLightTransform->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        //pointLightTransform->SetOrientation(glm::vec3(45.0f, 0.0f, 0.0f));
        //pointLight1->diffuse = glm::vec3(0.0f, 1.0f, 0.0f);
        //pointLight1->specular = glm::vec3(0.0f, 1.0f, 0.0f);
        //pointLight1->SetDistanceEffect(100.0f);
        //this->gameObjectManager->AddGameObject(pointLight, "LuzPuntualGO");

        //this->lightManager->CreateLight(LightType::POINT_LIGHT, "PointLight2");
        //auto pointLight2 = this->lightManager->GetLight("PointLight2");
        //pointLight2->transform->SetPosition(glm::vec3(0.0f, 5.0f, 5.0f));
        //pointLight2->diffuse = glm::vec3(0.3f, 0.3f, 0.7f);
        //pointLight2->specular = glm::vec3(0.3f, 0.3f, 0.7f);
        //pointLight2->SetDistanceEffect(100.0f);

        //this->lightManager->CreateLight(LightType::POINT_LIGHT, "PointLight3");
        //auto pointLight3 = this->lightManager->GetLight("PointLight3");
        //pointLight3->transform->SetPosition(glm::vec3(5.0f, 5.0f, 0.0f));
        //pointLight3->diffuse = glm::vec3(0.3f, 0.3f, 0.7f);
        //pointLight3->specular = glm::vec3(0.3f, 0.3f, 0.7f);
        //pointLight3->SetDistanceEffect(100.0f);
    }

    // DIRECTIONAL LIGHTS
    {
        //this->lightManager->CreateLight(LightType::DIRECTIONAL_LIGHT, "DirectionalLight0");
        //auto dirlight = this->lightManager->GetLight("DirectionalLight0");
        //dirlight->diffuse = glm::vec3(0.6f);
        //dirlight->specular = glm::vec3(0.1f);
        //dirlight->SetDistanceEffect(100.0f);

        //this->lightManager->CreateLight(LightType::DIRECTIONAL_LIGHT, "DirectionalLight2");
        //auto dirlight2 = this->lightManager->GetLight("DirectionalLight2");
        //dirlight2->transform->SetOrientation(glm::vec3(45.0f, 0.0f, 0.0f));
        //dirlight2->diffuse = glm::vec3(0.3f, 0.3f, 0.7f);
        //dirlight2->specular = glm::vec3(0.3f, 0.3f, 0.7f);
        //dirlight2->SetDistanceEffect(100.0f);
    }


    //this->lightManager->CreateLight(LightType::SPOT_LIGHT, "SpotLight0");
    //auto spotLight = this->lightManager->GetLight("SpotLight0");
    //spotLight->transform->SetPosition(glm::vec3(0.0f, 5.0f, 5.0f));// -5.0f));
    //spotLight->transform->SetOrientation(glm::vec3(45.0f, 0.0f, 0.0f));
    //spotLight->diffuse = glm::vec3(0.6f);
    //spotLight->specular = glm::vec3(0.1f);
    //spotLight->cutOff = glm::cos(glm::radians(32.5f));
    //spotLight->outerCutoff = glm::cos(glm::radians(45.0f));
    //spotLight->SetDistanceEffect(100.0f);

    // END -------------------------- Lights ----------------------------------------

    this->sessionManager->RegisterActiveSceneCamera();

    this->physicsModule->SetGravity(-20.0f);

    // Initialize Managers
    this->gameObjectManager->StartQEGameObjects();

    // Initialize animation states for character controller
    auto animationComponent = character->GetComponent<AnimationComponent>();

    // States
    animationComponent->AddAnimationState({ "Idle", true, "Idle" }, /*isEntry*/ true);
    animationComponent->AddAnimationState({ "Walk", true, "Walking" });
    animationComponent->AddAnimationState({ "Attack", false, "Punch" });
    animationComponent->AddAnimationState({ "Jump", false, "Jumping" });

    // Parameters
    animationComponent->SetFloat("speed", 0.0f);
    animationComponent->SetTrigger("attack", false);
    animationComponent->SetTrigger("jump", false);

    //Transitions
    animationComponent->AddTransition({
        .fromState = "Idle", .toState = "Walk",
        .conditions = {{"speed", QEOp::Greater, 0.1f}},
        .priority = 10, .hasExitTime = false
    });
    animationComponent->AddTransition({
        .fromState = "Walk", .toState = "Idle",
        .conditions = {{"speed", QEOp::Less, 0.05f}},
        .priority = 10, .hasExitTime = true, .exitTimeNormalized = 0.2f
    });

    animationComponent->AddTransition({
        .fromState = "Idle", .toState = "Attack",
        .conditions = {{"attack", QEOp::Equal, 1.f}}, // trigger
        .priority = 100, .hasExitTime = false
    });
    animationComponent->AddTransition({
        .fromState = "Attack", .toState = "Idle",
        .conditions = {}, .priority = 10,
        .hasExitTime = true, .exitTimeNormalized = 1.0f
    });

    animationComponent->AddTransition({
        .fromState = "Idle", .toState = "Jump",
        .conditions = {{"jump", QEOp::Equal, 1.f}}, // trigger
        .priority = 10, .hasExitTime = false
    });
    animationComponent->AddTransition({
        .fromState = "Jump", .toState = "Idle",
        .conditions = {}, .priority = 10,
        .hasExitTime = true, .exitTimeNormalized = 1.0f
    });

    this->gameObjectManager->UpdateQEGameObjects();

    this->lightManager->InitializeShadowMaps();
    this->atmosphereSystem->InitializeAtmosphereResources();

    this->commandPoolModule->Render(&framebufferModule);
    this->synchronizationModule.createSyncObjects();

    init_imgui();

    //auto yaml = wall->ToYaml();
    //exportToFile(yaml, "wallGameObject.yaml");
    //std::cout << yaml << std::endl;
    /**/
}

void App::loadScene(QEScene scene)
{
    scene.DeserializeScene();

    // Initialize active camera resources
    this->sessionManager->ActiveCamera()->QEStart();

    // Initialize the light manager & the lights
    this->lightManager->AddDirShadowMapShader(materialManager->csm_shader);
    this->lightManager->AddOmniShadowMapShader(materialManager->omni_shadow_mapping_shader);

    // Initialize the atmophere system
    this->atmosphereSystem = AtmosphereSystem::getInstance();
    this->atmosphereSystem->LoadAtmosphereDto(scene.atmosphereDto);
}

void App::saveScene()
{
    this->scene.cameraEditor = this->sessionManager->EditorCamera();
    this->scene.atmosphereDto = this->atmosphereSystem->CreateAtmosphereDto();
    this->scene.SerializeScene();
}

void App::mainLoop()
{
    bool changeAnimation = true;

    while (!glfwWindowShouldClose(mainWindow->getWindow()))
    {
        glfwPollEvents();
        this->timer->UpdateDeltaTime();

        //PHYSIC SYSTEM
        this->physicsModule->ComputePhysics((float)Timer::DeltaTime);

        this->gameObjectManager->UpdateQEGameObjects();
                
        // INPUT EVENTS
        this->keyboard_ptr->ReadKeyboardEvents();

        this->sessionManager->UpdateActiveCamera((float)Timer::DeltaTime);

        // UPDATE CULLING SCENE
        this->sessionManager->UpdateCullingScene();

        // UPDATE LIGHT SYSTEM
        this->lightManager->Update();

        // UPDATE ATMOSPHERE
        this->atmosphereSystem->UpdateSun();
        auto sunLight = std::static_pointer_cast<QESunLight>(this->lightManager->GetLight("QESunLight"));

        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            saveScene();
        }

        glm::vec3 newDir = sunLight->transform->Forward();
        if (ImGui::IsKeyDown(ImGuiKey_J))
        {
            newDir.x += 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown(ImGuiKey_L))
        {
            newDir.x -= 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown(ImGuiKey_I))
        {
            newDir.y += 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown(ImGuiKey_K))
        {
            newDir.y -= 0.001f;
            sunLight->SetLightDirection(newDir);
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

            this->computeFrame();
            this->drawFrame();
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

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(deviceModule->device, imguiPool, nullptr);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::cleanUp()
{
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
    this->sessionManager->CleanEditorResources();
    this->sessionManager->CleanCullingResources();
    this->physicsModule->CleanupDebugDrawer();

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

void App::cleanUpSwapchain()
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

void App::cleanManagers()
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

void App::computeFrame()
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

void App::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule->getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &swapchainModule->currentImage);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    //this->cameraEditor->UpdateUBOCamera();
    //this->lightManager->UpdateUBOLight();
    this->materialManager->UpdateUniforms();

    vkDeviceWaitIdle(deviceModule->device);

    commandPoolModule->Render(&framebufferModule);

    synchronizationModule.submitCommandBuffer(commandPoolModule->getCommandBuffer((uint32_t)synchronizationModule.GetCurrentFrame()), this->isRender);

    result = synchronizationModule.presentSwapchain(swapchainModule->getSwapchain(), swapchainModule->currentImage);
    resizeSwapchain(result, ERROR_RESIZE::IMAGE_ERROR);
    this->isRender = true;
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
    mainWindow->checkMinimize();

    vkDeviceWaitIdle(deviceModule->device);

    //Recreamos el swapchain
    cleanUpSwapchain();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow->getWindow());

    //Actualizamos el formato de la cámara
    this->sessionManager->UpdateViewportSize();

    //Actualizamos la resolución de la atmosfera
    this->atmosphereSystem->UpdateAtmopshereResolution();

    //Recreamos el antialiasing module
    antialiasingModule->createColorResources();
    //Recreamos el depth buffer module
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Recreamos el render pass
    renderPassModule->CreateImGuiRenderPass(swapchainModule->swapChainImageFormat, *antialiasingModule->msaaSamples);
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
}
