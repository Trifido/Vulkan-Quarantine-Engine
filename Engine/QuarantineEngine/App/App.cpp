#include "App.h"

#include "SyncTool.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

App::App()
{
    deltaTime = 0;
    lastFrame = 0.0f;
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
    float currentFrame = glfwGetTime();
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
    pool_info.poolSizeCount = std::size(pool_sizes);
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

    ImGui_ImplVulkan_Init(&init_info, graphicsPipelineModule.renderPass);

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
    queueModule = QueueModule::getInstance();
    deviceModule = DeviceModule::getInstance();
    deviceModule->pickPhysicalDevice(vulkanInstance.getInstance(), windowSurface.getSurface());
    deviceModule->createLogicalDevice(windowSurface.getSurface(), *queueModule);
    swapchainModule.createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());
    imageViewModule.createImageViews(swapchainModule);

    commandPoolModule = CommandPoolModule::getInstance();
    commandPoolModule->createCommandPool(windowSurface.getSurface());

    antialiasingModule.createColorResources(swapchainModule);

    depthBufferModule.addAntiAliasingModule(antialiasingModule);
    depthBufferModule.createDepthResources(swapchainModule.swapChainExtent, commandPoolModule->getCommandPool());

    model = std::make_shared<GameObject>(GameObject(MODEL_PATH, TEXTURE_PATH, swapchainModule.getNumSwapChainImages(), commandPoolModule->getCommandPool()));

    //raytracingModule.addModules(bufferModule, queueModule);
    //raytracingModule.initRayTracing();

    shaderModule.createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv");
    graphicsPipelineModule.addAntialiasingModule(antialiasingModule);
    graphicsPipelineModule.addShaderModules(shaderModule);
    graphicsPipelineModule.createRenderPass(swapchainModule.swapChainImageFormat, depthBufferModule);
    graphicsPipelineModule.createGraphicsPipeline(swapchainModule.swapChainExtent, model->descriptorModule->getDescriptorSetLayout());

    framebufferModule.addAntialiasingModule(antialiasingModule);
    framebufferModule.createFramebuffer(graphicsPipelineModule.renderPass, imageViewModule.swapChainImageViews, swapchainModule.swapChainExtent, depthBufferModule);
    commandPoolModule->createCommandBuffers(framebufferModule.swapChainFramebuffers, graphicsPipelineModule.renderPass, swapchainModule.swapChainExtent,
                                            graphicsPipelineModule.pipelineLayout, graphicsPipelineModule.graphicsPipeline,
                                            *model);

    synchronizationModule.createSyncObjects(swapchainModule.getNumSwapChainImages());

    init_imgui();
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(mainWindow.getWindow())) {
        glfwPollEvents();
        computeDeltaTime();
        camera_ptr->CameraController(deltaTime);


        //ImGui_ImplGlfw_NewFrame();
        //ImGui::NewFrame();

        //bool show_another_window = true;
        //ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //ImGui::Text("Hello from another window!");
        //if (ImGui::Button("Close Me"))
        //    show_another_window = false;
        //ImGui::End();

        //ImGui::Render();

        //if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        //{
        //    ImGui::UpdatePlatformWindows();
        //    ImGui::RenderPlatformWindowsDefault();
        //}


        //your draw function
        //drawFrame();

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

    }
    vkDeviceWaitIdle(deviceModule->device);

    vkDestroyDescriptorPool(deviceModule->device, imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    //fontModule.cleanup(deviceModule->device);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::cleanUp()
{
    cleanUpSwapchain();

    model->cleanup();
    model->descriptorModule->cleanup();

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
    antialiasingModule.cleanup();
    depthBufferModule.cleanup();
    framebufferModule.cleanup();

    vkFreeCommandBuffers(deviceModule->device, commandPoolModule->getCommandPool(), commandPoolModule->getNumCommandBuffers(), commandPoolModule->getCommandBuffers().data());

    graphicsPipelineModule.cleanup();
    imageViewModule.cleanup();
    swapchainModule.cleanup();

    model->descriptorModule->cleanupDescriptorBuffer();
    model->descriptorModule->cleanupDescriptorPool();
}

void App::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule.getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    synchronizationModule.synchronizeCurrentFrame(imageIndex);

    model->descriptorModule->updateUniformBuffer(imageIndex, swapchainModule.swapChainExtent, model->transform);
    vkDeviceWaitIdle(deviceModule->device);
    commandPoolModule->recreateCommandBuffers(framebufferModule.swapChainFramebuffers, graphicsPipelineModule.renderPass, swapchainModule.swapChainExtent,
        graphicsPipelineModule.pipelineLayout, graphicsPipelineModule.graphicsPipeline,
        *model);

    synchronizationModule.submitCommandBuffer(commandPoolModule->getCommandBuffer(imageIndex));

    result = synchronizationModule.presentSwapchain(swapchainModule.getSwapchain(), imageIndex);
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

    cleanUpSwapchain();

    swapchainModule.createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());
    imageViewModule.createImageViews(swapchainModule);
    shaderModule.createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv");
    antialiasingModule.createColorResources(swapchainModule);
    depthBufferModule.createDepthResources(swapchainModule.swapChainExtent, commandPoolModule->getCommandPool());
    graphicsPipelineModule.addShaderModules(shaderModule);
    graphicsPipelineModule.createRenderPass(swapchainModule.swapChainImageFormat, depthBufferModule);
    graphicsPipelineModule.createGraphicsPipeline(swapchainModule.swapChainExtent, model->descriptorModule->getDescriptorSetLayout());
    framebufferModule.createFramebuffer(graphicsPipelineModule.renderPass, imageViewModule.swapChainImageViews, swapchainModule.swapChainExtent, depthBufferModule);

    model->descriptorModule->recreateUniformBuffer(swapchainModule.getNumSwapChainImages());
    commandPoolModule->createCommandBuffers(framebufferModule.swapChainFramebuffers, graphicsPipelineModule.renderPass, swapchainModule.swapChainExtent,
        graphicsPipelineModule.pipelineLayout, graphicsPipelineModule.graphicsPipeline,
        *model);
}

/*
bool App::createFontsTexture(VkCommandBuffer& commandBuffer)
{
    unsigned char* pixels;
    int width, height;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width * height * 4 * sizeof(char);

    VkResult err;

    // Create the Image:
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        err = vkCreateImage(deviceModule->device, &info, nullptr, &fontModule.fontImage);

        VkMemoryRequirements req;
        vkGetImageMemoryRequirements(deviceModule->device, fontModule.fontImage, &req);
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = IMT::findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceModule->physicalDevice);
            //ImGui_ImplVulkan_MemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
        err = vkAllocateMemory(deviceModule->device, &alloc_info, nullptr, &fontModule.fontMemory);
        err = vkBindImageMemory(deviceModule->device, fontModule.fontImage, fontModule.fontMemory, 0);
    }

    // Create the Image View:
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = fontModule.fontImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        err = vkCreateImageView(deviceModule->device, &info, nullptr, &fontModule.fontView);
    }

    // Update the Descriptor Set:
    //{
    //    VkDescriptorImageInfo desc_image[1] = {};
    //    desc_image[0].sampler = fontModule.fontSampler;
    //    desc_image[0].imageView = fontModule.fontView;
    //    desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //    VkWriteDescriptorSet write_desc[1] = {};
    //    write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //    write_desc[0].dstSet = g_DescriptorSet;
    //    write_desc[0].descriptorCount = 1;
    //    write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //    write_desc[0].pImageInfo = desc_image;
    //    vkUpdateDescriptorSets(deviceModule->device, 1, write_desc, 0, NULL);
    //}

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = upload_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer(deviceModule->device, &buffer_info, nullptr, &fontModule.uploadBuffer);
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(deviceModule->device, fontModule.uploadBuffer, &req);
        fontModule.bufferMemoryAlignment = (fontModule.bufferMemoryAlignment > req.alignment) ? fontModule.bufferMemoryAlignment : req.alignment;
        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = req.size;
        alloc_info.memoryTypeIndex = fontModule.getMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits, deviceModule->physicalDevice);
        err = vkAllocateMemory(deviceModule->device, &alloc_info, nullptr, &fontModule.uploadBufferMemory);
        err = vkBindBufferMemory(deviceModule->device, fontModule.uploadBuffer, fontModule.uploadBufferMemory, 0);
    }

    // Upload to Buffer:
    {
        char* map = NULL;
        err = vkMapMemory(deviceModule->device, fontModule.uploadBufferMemory, 0, upload_size, 0, (void**)(&map));
        memcpy(map, pixels, upload_size);
        VkMappedMemoryRange range[1] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = fontModule.uploadBufferMemory;
        range[0].size = upload_size;
        err = vkFlushMappedMemoryRanges(deviceModule->device, 1, range);
        vkUnmapMemory(deviceModule->device, fontModule.uploadBufferMemory);
    }

    // Copy to Image:
    {
        VkImageMemoryBarrier copy_barrier[1] = {};
        copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[0].image = fontModule.fontImage;
        copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[0].subresourceRange.levelCount = 1;
        copy_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(commandBuffer, fontModule.uploadBuffer, fontModule.fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier use_barrier[1] = {};
        use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[0].image = fontModule.fontImage;
        use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[0].subresourceRange.levelCount = 1;
        use_barrier[0].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
    }

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)fontModule.fontImage);

    return true;
}

void App::setupImguiUploadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\Arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    IM_ASSERT(font != NULL);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, commandPoolModule->getCommandPool());
    createFontsTexture(commandBuffer);
    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, commandPoolModule->getCommandPool(), commandBuffer);
}
*/
