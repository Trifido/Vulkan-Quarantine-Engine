#include "App.h"

App::App()
{
}

void App::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void App::initWindow()
{
    mainWindow.init();
    //camera.AddWindow(*mainWindow.getWindow());
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
}

void App::mainLoop()
{
    while (!glfwWindowShouldClose(mainWindow.getWindow())) {
        glfwPollEvents();

        drawFrame();
    }
    vkDeviceWaitIdle(deviceModule->device);
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
    //bufferModule.updateUniformBufferCamera(imageIndex, swapchainModule.swapChainExtent, camera);

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