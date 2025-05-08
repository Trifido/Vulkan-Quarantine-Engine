#include "App.h"

#include "SyncTool.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <BufferManageModule.h>
#include <filesystem>
#include "../Editor/Grid.h"
#include <QEProjectManager.h>


App::App()
{
    this->timer = Timer::getInstance();
    this->keyboard_ptr = KeyboardController::getInstance();
    this->queueModule = QueueModule::getInstance();
    this->deviceModule = DeviceModule::getInstance();

    this->physicsModule = PhysicsModule::getInstance();
    this->editorManager = EditorObjectManager::getInstance();
    this->animationManager = AnimationManager::getInstance();
    this->graphicsPipelineManager = GraphicsPipelineManager::getInstance();
    this->shadowPipelineManager = ShadowPipelineManager::getInstance();
    this->computePipelineManager = ComputePipelineManager::getInstance();
}

App::~App()
{

}

void App::run(QEScene scene)
{
    this->scene = scene;

    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void App::initWindow()
{
    this->mainWindow.init();
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

    ImGui_ImplVulkan_Init(&init_info, *(renderPassModule->renderPass));

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

    //Inicializamos el CommandPool Module
    commandPoolModule = CommandPoolModule::getInstance();
    commandPoolModule->ClearColor = glm::vec3(0.1f);

    //Inicializamos el Swapchain Module
    swapchainModule = SwapChainModule::getInstance();
    swapchainModule->InitializeScreenDataResources();
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

    //Creamos el Render Pass
    renderPassModule = RenderPassModule::getInstance();
    renderPassModule->createRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);
    renderPassModule->createDirShadowRenderPass(VK_FORMAT_D32_SFLOAT);
    renderPassModule->createOmniShadowRenderPass(VK_FORMAT_R32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

    //Registramos el default render pass
    this->graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->renderPass);

    //Creamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->renderPass);

    //Añadimos requisitos para los geometryComponent
    BufferManageModule::commandPool = this->commandPoolModule->getCommandPool();
    BufferManageModule::computeCommandPool = this->commandPoolModule->getComputeCommandPool();
    BufferManageModule::graphicsQueue = this->queueModule->graphicsQueue;
    BufferManageModule::computeQueue = this->queueModule->computeQueue;
    GeometryComponent::deviceModule_ptr = this->deviceModule;
    TextureManagerModule::queueModule = this->queueModule;
    CustomTexture::commandPool = commandPoolModule->getCommandPool();
    OmniShadowResources::commandPool = commandPoolModule->getCommandPool();
    CSMResources::commandPool = commandPoolModule->getCommandPool();
    OmniShadowResources::queueModule = this->queueModule;
    CSMResources::queueModule = this->queueModule;

    // INIT ------------------------- Mesh & Material -------------------------------
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

    // Load Scene
    this->loadScene(this->scene);

    this->cullingSceneManager = CullingSceneManager::getInstance();
    this->cullingSceneManager->InitializeCullingSceneResources();
    this->cullingSceneManager->AddCameraFrustum(this->cameraEditor->frustumComponent);
    this->cullingSceneManager->DebugMode = false;

    this->physicsModule->InitializeDebugResources();
    this->physicsModule->debugDrawer->DebugMode = false;

    // Inicializamos los componentes del editor
    std::shared_ptr<Grid> grid_ptr = std::make_shared<Grid>();
    this->editorManager->AddEditorObject(grid_ptr, "editor:grid");
    grid_ptr->IsRenderable = true;

    auto absPath = std::filesystem::absolute("../../resources/models").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos)
    {
        absPath.erase(ind, substring.length());
    }

    //const std::string absolute_path = absPath + "/cyber_warrior/scene.gltf";
    //const std::string absolute_path = absPath + "/drone/mech_drone.glb";
    //const std::string absolute_path = absPath + "/newell_teaset/teapot.obj";
    //const std::string absolute_path = absPath + "/Raptoid/scene.gltf";

    //std::filesystem::path path = "C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/QEProjects/QEExample/QEAssets/QEModels/golem/Meshes/scene.gltf";
    //std::filesystem::path path = "C:/Users/Usuario/Documents/GitHub/Vulkan-Quarantine-Engine/QEProjects/QEExample/QEAssets/QEModels/Raptoid/Meshes/scene.gltf";
    //std::shared_ptr<GameObject> model = std::make_shared<GameObject>(GameObject(path.string()));

    //model->_Transform->SetPosition(glm::vec3(-3.5f, 1.3f, -2.0f));
    //model->transform->SetOrientation(glm::vec3(-90.0f, 180.0f, 0.0f));
    //model->_Transform->SetScale(glm::vec3(0.01f));
    //model->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.7f, 0.2f));
    //model->_Material->materialData.SetMaterialField("Specular", glm::vec3(0.5f, 0.5f, 0.5f));
    //model->_Material->materialData.SetMaterialField("Ambient", glm::vec3(0.2f));
    //this->gameObjectManager->AddGameObject(model, "modelRaptoid");

    /*
    std::shared_ptr<GameObject> floor = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::PLANE_TYPE));
    floor->_Transform->SetPosition(glm::vec3(0.0f, -0.01f, 0.0f));
    floor->_Transform->SetScale(glm::vec3(10.0f, 1.0f, 10.0f));
    floor->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.2f, 0.7f));
    floor->_Material->materialData.SetMaterialField("Specular", glm::vec3(0.0f, 0.0f, 0.0f));
    floor->_Material->materialData.SetMaterialField("Ambient", glm::vec3(0.2f));
    floor->AddPhysicBody(std::make_shared<PhysicBody>());
    floor->physicBody->CollisionGroup = CollisionFlag::COL_SCENE;
    floor->physicBody->CollisionMask = CollisionFlag::COL_PLAYER;
    floor->AddCollider(std::make_shared<PlaneCollider>());
    std::static_pointer_cast<PlaneCollider>(floor->collider)->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    this->gameObjectManager->AddGameObject(floor, "floor");
    /*
    std::shared_ptr<GameObject> ramp = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::CUBE_TYPE));
    ramp->_Transform->SetPosition(glm::vec3(0.0f, -0.5f, -10.0f));
    ramp->_Transform->SetOrientation(glm::vec3(70.0f, 0.0f, 0.0f));
    ramp->_Transform->SetScale(glm::vec3(3.0f, 10.0f, 3.0f));
    ramp->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.2f, 0.7f, 0.2f));
    ramp->AddPhysicBody(std::make_shared<PhysicBody>());
    ramp->physicBody->CollisionGroup = CollisionFlag::COL_SCENE;
    ramp->physicBody->CollisionMask = CollisionFlag::COL_PLAYER;
    ramp->AddCollider(std::make_shared<BoxCollider>());
    this->gameObjectManager->AddGameObject(ramp, "ramp");
    /**/
    std::shared_ptr<GameObject> wall = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::CUBE_TYPE));
    wall->_Transform->SetPosition(glm::vec3(3.0f, 0.5f, 0.0f));
    wall->_Transform->SetOrientation(glm::vec3(0.0f, 0.0f, 0.0f));
    wall->_Transform->SetScale(glm::vec3(4.0f, 0.5f, 4.0f));
    wall->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.8f, 0.2f, 0.2f));
    wall->AddPhysicBody(std::make_shared<PhysicBody>());
    wall->physicBody->CollisionGroup = CollisionFlag::COL_SCENE;
    wall->physicBody->CollisionMask = CollisionFlag::COL_PLAYER;
    wall->AddCollider(std::make_shared<BoxCollider>());
    this->gameObjectManager->AddGameObject(wall, "wall");
    /*
    // CHARACTER CONTROLLER
    std::shared_ptr<GameObject> character = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::CAPSULE_TYPE));
    character->_Transform->SetPosition(glm::vec3(0.0f, 0.5f, 0.0f));
    character->_Material->materialData.SetMaterialField("Diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    character->AddPhysicBody(std::make_shared<PhysicBody>(PhysicBodyType::RIGID_BODY));
    character->AddCollider(std::make_shared<CapsuleCollider>());
    character->physicBody->Mass = 70.0f;
    character->physicBody->CollisionGroup = CollisionFlag::COL_PLAYER;
    character->physicBody->CollisionMask = CollisionFlag::COL_SCENE;
    character->AddCharacterController(std::make_shared<QECharacterController>());
    character->characterController->SetJumpForce(9.0f);

    this->gameObjectManager->AddGameObject(character, "character");
    /**/

//DEMOD
/*
    //Creamos la textura
    //textureManager->AddTexture("diffuse_brick", CustomTexture(TEXTURE_WALL_PATH, TEXTURE_TYPE::DIFFUSE_TYPE));
    //textureManager->AddTexture("normal_brick", CustomTexture(TEXTURE_WALL_NORMAL_PATH, TEXTURE_TYPE::NORMAL_TYPE));
    //textureManager->AddTexture("test", CustomTexture(TEXTURE_TEST_PATH, TEXTURE_TYPE::DIFFUSE_TYPE));

    std::shared_ptr<GameObject> cube = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::CUBE_TYPE));
    cube->_Transform->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    cube->_Transform->SetOrientation(glm::vec3(0.0f, 0.0f, 65.0f));

    std::shared_ptr<GameObject> plano = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::PLANE_TYPE));
    plano->_Transform->SetOrientation(glm::vec3(0.0f, 0.0f, 45.0f));
    plano->_Transform->SetPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    plano->_Transform->SetScale(glm::vec3(5.0f, 1.0f, 5.0f));

    std::shared_ptr<GameObject> floor = std::make_shared<GameObject>(GameObject(PRIMITIVE_TYPE::PLANE_TYPE));
    floor->_Transform->SetPosition(glm::vec3(0.0f, -0.10f, 0.0f));
    floor->_Transform->SetScale(glm::vec3(50.0f, 1.0f, 50.0f));

    //Creamos el shader module para el material
    //std::shared_ptr<ShaderModule> shader_ptr = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv"));
    //shader_ptr->createShaderBindings();
    //this->shaderManager->AddShader("shader", shader_ptr);

    ////Creamos el material
    //std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(Material(this->shaderManager->GetShader("shader"), renderPassModule->renderPass));
    //mat_ptr->AddNullTexture(textureManager->GetTexture("NULL"));
    //mat_ptr->AddTexture(textureManager->GetTexture("diffuse_brick"));
    //mat_ptr->AddTexture(textureManager->GetTexture("normal_brick"));

    //std::shared_ptr<Material> mat_ptr2 = std::make_shared<Material>(Material(this->shaderManager->GetShader("shader"), renderPassModule->renderPass));
    //mat_ptr2->AddNullTexture(textureManager->GetTexture("NULL"));
    //mat_ptr2->AddTexture(textureManager->GetTexture("test"));

    //materialManager->AddMaterial("mat", mat_ptr);
    //materialManager->AddMaterial("mat2", mat_ptr2);

    //Linkamos el material al gameobject
    //cube->AddMaterial(materialManager->GetMaterial("mat"));
    //plano->AddMaterial(materialManager->GetMaterial("mat"));
    //floor->AddMaterial(materialManager->GetMaterial("mat2"));

    
/**/
    // END -------------------------- Mesh & Material -------------------------------

    // INIT ------------------------- Lights ----------------------------------------
    // POINT LIGHTS
    {
        //this->lightManager->CreateLight(LightType::POINT_LIGHT, "PointLight1");
        //auto pointLight = this->lightManager->GetLight("PointLight1");
        //pointLight->transform->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        //pointLight->diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
        //pointLight->specular = glm::vec3(0.7f, 0.7f, 0.7f);
        //pointLight->SetDistanceEffect(100.0f);

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

    // Initialize Physics
    /*
    std::shared_ptr<PhysicBody> rigidBody = std::make_shared<PhysicBody>(PhysicBody(PhysicBodyType::RIGID_BODY));
    rigidBody->Mass = 10.0f;// 0.001f;
    cube->AddPhysicBody(rigidBody);
    std::shared_ptr<BoxCollider> boxCollider = std::make_shared<BoxCollider>();
    cube->AddCollider(boxCollider);

    std::shared_ptr<PhysicBody> staticBody = std::make_shared<PhysicBody>();
    plano->AddPhysicBody(staticBody);
    std::shared_ptr<PlaneCollider> planeCollider = std::make_shared<PlaneCollider>();
    planeCollider->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    plano->AddCollider(planeCollider);

    std::shared_ptr<PhysicBody> staticBody2 = std::make_shared<PhysicBody>();
    floor->AddPhysicBody(staticBody2);
    std::shared_ptr<PlaneCollider> planeCollider2 = std::make_shared<PlaneCollider>();
    planeCollider2->SetPlane(0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
    floor->AddCollider(planeCollider2);

    this->gameObjectManager->AddGameObject(cube, "cube");
    this->gameObjectManager->AddGameObject(plano, "planoInclinado");
    this->gameObjectManager->AddGameObject(floor, "floor");
    /**/

    this->physicsModule->SetGravity(-20.0f);

    // Initialize Managers
    this->animationManager->InitializeAnimations();
    this->animationManager->UpdateAnimations(0.0f);
    this->gameObjectManager->InitializePhysics();
    this->materialManager->InitializeMaterials();
    this->computeNodeManager->InitializeComputeNodes();
    this->lightManager->InitializeShadowMaps();

    this->commandPoolModule->Render(&framebufferModule);
    this->synchronizationModule.createSyncObjects();

    init_imgui();
}

void App::loadScene(QEScene scene)
{
    // Initialize the camera editor
    this->cameraEditor = CameraEditor::getInstance();
    this->cameraEditor->LoadCameraDto(this->mainWindow.width, this->mainWindow.height, this->scene.cameraEditor);

    // Initialize the materials
    this->materialManager->LoadMaterialDtos(this->scene.materialDtos);

    // Initialize the game object manager & the game objects
    cout << "GameObject loading..." << endl;
    this->gameObjectManager->LoadGameObjectDtos(this->scene.gameObjectDtos);

    // Initialize the light manager & the lights
    this->lightManager->AddDirShadowMapShader(materialManager->csm_shader);
    this->lightManager->AddOmniShadowMapShader(materialManager->omni_shadow_mapping_shader);
    this->lightManager->SetCamera(this->cameraEditor);
    this->lightManager->LoadLightDtos(this->scene.lightDtos);

    // Initialize the atmophere system
    this->atmosphereSystem = AtmosphereSystem::getInstance();
    this->atmosphereSystem->LoadAtmosphereDto(this->scene.atmosphere, this->cameraEditor);
}

void App::mainLoop()
{
    bool changeAnimation = true;

    while (!glfwWindowShouldClose(mainWindow.getWindow()))
    {
        glfwPollEvents();
        this->timer->UpdateDeltaTime();

        //UPDATE CHARACTER CONTROLLER
        //auto character = this->gameObjectManager->GetGameObject("character");
        //character->characterController->Update();
        //QECharacterController::ProcessInput(mainWindow.getWindow(), character->characterController);

        //PHYSIC SYSTEM
        this->physicsModule->ComputePhysics((float)Timer::DeltaTime);

        // Update transforms
        this->gameObjectManager->UpdatePhysicTransforms();

        //ANIMATION SYSTEM
        this->animationManager->UpdateAnimations((float)Timer::DeltaTime);
        
        //COMPUTE NODES
        this->computeNodeManager->UpdateComputeNodes();

        // INPUT EVENTS
        this->keyboard_ptr->ReadKeyboardEvents();
        this->cameraEditor->CameraController((float)Timer::DeltaTime);

        // UPDATE CULLING SCENE
        this->cullingSceneManager->UpdateCullingScene();

        // UPDATE LIGHT SYSTEM
        this->lightManager->Update();

        // UPDATE ATMOSPHERE
        this->atmosphereSystem->UpdateSun();
        auto sunLight = std::static_pointer_cast<SunLight>(this->lightManager->GetLight("QESunLight"));

        ImGuiIO& io = ImGui::GetIO();
        if (io.KeyCtrl && (ImGui::IsKeyPressed('s', false) || ImGui::IsKeyPressed('S', false)))
        {
            this->scene.cameraEditor = this->cameraEditor->CreateCameraDto();
            this->scene.atmosphere = this->atmosphereSystem->CreateAtmosphereDto();
            this->scene.SaveScene();
        }

        if (ImGui::IsKeyDown('j') || ImGui::IsKeyDown('J'))
        {
            glm::vec3 newDir = sunLight->transform->ForwardVector;
            newDir.x += 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown('l') || ImGui::IsKeyDown('L'))
        {
            glm::vec3 newDir = sunLight->transform->ForwardVector;
            newDir.x -= 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown('I'))
        {
            glm::vec3 newDir = sunLight->transform->ForwardVector;
            newDir.y += 0.001f;
            sunLight->SetLightDirection(newDir);
        }
        if (ImGui::IsKeyDown('K'))
        {
            glm::vec3 newDir = sunLight->transform->ForwardVector;
            newDir.y -= 0.001f;
            sunLight->SetLightDirection(newDir);
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

            this->computeFrame();
            this->drawFrame();
        }

        this->cameraEditor->ResetModifiedField();
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
    this->shaderManager->Clean();

    //this->OmniShadowResources->cleanup();
    this->cleanUpSwapchain();
    //this->framebufferModule.cleanupShadowBuffer();
    this->swapchainModule->CleanScreenDataResources();

    this->materialManager->CleanPipelines();
    this->computePipelineManager->CleanComputePipeline();
    this->computeNodeManager->Cleanup();
    this->animationManager->Cleanup();
    this->lightManager->CleanShadowMapResources();

    this->atmosphereSystem->Cleanup();
    this->gameObjectManager->Cleanup();
    this->particleSystemManager->Cleanup();
    this->editorManager->Cleanup();
    this->cullingSceneManager->CleanUp();
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

    glfwDestroyWindow(mainWindow.getWindow());

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

    this->animationManager->ResetInstance();
    this->animationManager = nullptr;

    this->atmosphereSystem->CleanLastResources();
    this->atmosphereSystem->ResetInstance();
    this->atmosphereSystem = nullptr;

    this->gameObjectManager->CleanLastResources();
    this->gameObjectManager->ResetInstance();
    this->gameObjectManager = nullptr;

    this->cullingSceneManager->ResetInstance();
    this->cullingSceneManager = nullptr;

    this->particleSystemManager->CleanLastResources();
    this->particleSystemManager->ResetInstance();
    this->particleSystemManager = nullptr;

    this->textureManager->CleanLastResources();
    this->textureManager->ResetInstance();
    this->textureManager = nullptr;

    this->editorManager->CleanLastResources();
    this->editorManager->ResetInstance();
    this->editorManager = nullptr;

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

    delete this->cameraEditor;
    this->cameraEditor = nullptr;

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
    mainWindow.checkMinimize();

    vkDeviceWaitIdle(deviceModule->device);

    //Recreamos el swapchain
    cleanUpSwapchain();
    swapchainModule->createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());

    //Actualizamos el formato de la cámara
    this->cameraEditor->UpdateViewportSize(swapchainModule->swapChainExtent);
    this->cameraEditor->UpdateCamera();

    //Actualizamos la resolución de la atmosfera
    this->atmosphereSystem->UpdateAtmopshereResolution();

    //Recreamos el antialiasing module
    antialiasingModule->createColorResources();
    //Recreamos el depth buffer module
    depthBufferModule->createDepthResources(swapchainModule->swapChainExtent, commandPoolModule->getCommandPool());

    //Recreamos el render pass
    renderPassModule->createRenderPass(swapchainModule->swapChainImageFormat, depthBufferModule->findDepthFormat(), *antialiasingModule->msaaSamples);
    renderPassModule->createDirShadowRenderPass(VK_FORMAT_D32_SFLOAT);
    renderPassModule->createOmniShadowRenderPass(VK_FORMAT_R32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT);

    //Recreamos los graphics pipeline de los materiales
    graphicsPipelineManager->RegisterDefaultRenderPass(renderPassModule->renderPass);

    shaderManager->RecreateShaderGraphicsPipelines();
    //materialManager->RecreateMaterials(renderPassModule);

    //Recreamos el frame buffer
    framebufferModule.createFramebuffer(renderPassModule->renderPass);

    commandPoolModule->recreateCommandBuffers();
    commandPoolModule->Render(&framebufferModule);
}
