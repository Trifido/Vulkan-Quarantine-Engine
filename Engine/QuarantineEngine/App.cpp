#include "App.h"

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
}

void App::initVulkan()
{
    vulkanInstance.debug_level = DEBUG_LEVEL::ONLY_ERROR;
    vulkanInstance.createInstance();
    layerExtensionModule.setupDebugMessenger(vulkanInstance.getInstance(), vulkanInstance.debug_level);
    windowSurface.createSurface(vulkanInstance.getInstance(), mainWindow.getWindow());
    deviceModule = DeviceModule::getInstance();
    deviceModule->pickPhysicalDevice(vulkanInstance.getInstance(), windowSurface.getSurface());
    deviceModule->createLogicalDevice(windowSurface.getSurface(), queueModule);
    swapchainModule.createSwapChain(windowSurface.getSurface(), mainWindow.getWindow());
    imageViewModule.createImageViews(swapchainModule);

    commandPoolModule = CommandPoolModule::getInstance();
    commandPoolModule->createCommandPool(windowSurface.getSurface());

    depthBufferModule.createDepthResources(swapchainModule.swapChainExtent, queueModule, *commandPoolModule);

    textureModule.createTextureImage(bufferModule, queueModule, *commandPoolModule);
    textureModule.createTextureImageView();
    textureModule.createTextureSampler();

    bufferModule.createVertexBuffer(geometryModule, queueModule);
    bufferModule.createIndexBuffer(geometryModule, queueModule);
    bufferModule.createUniformBuffers(swapchainModule.getNumSwapChainImages());

    descriptorModule.addPtrData(&bufferModule, &textureModule);
    descriptorModule.createDescriptorSetLayout();
    descriptorModule.createDescriptorPool(swapchainModule.getNumSwapChainImages());
    descriptorModule.createDescriptorSets();

    shaderModule.createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv", geometryModule);
    graphicsPipelineModule.addShaderModules(shaderModule);
    graphicsPipelineModule.createRenderPass(swapchainModule.swapChainImageFormat, depthBufferModule);
    graphicsPipelineModule.createGraphicsPipeline(swapchainModule.swapChainExtent, descriptorModule.getDescriptorSetLayout());

    framebufferModule.createFramebuffer(graphicsPipelineModule.renderPass, imageViewModule.swapChainImageViews, swapchainModule.swapChainExtent, depthBufferModule);
    commandPoolModule->createCommandBuffers(framebufferModule.swapChainFramebuffers, graphicsPipelineModule.renderPass, swapchainModule.swapChainExtent,
                                            graphicsPipelineModule.pipelineLayout, graphicsPipelineModule.graphicsPipeline,
                                            geometryModule, bufferModule, descriptorModule.getDescriptorSet());

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

    textureModule.cleanup();
    descriptorModule.cleanup();
    bufferModule.cleanup();
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
    depthBufferModule.cleanup();
    framebufferModule.cleanup();

    vkFreeCommandBuffers(deviceModule->device, commandPoolModule->getCommandPool(), commandPoolModule->getNumCommandBuffers(), commandPoolModule->getCommandBuffers().data());

    graphicsPipelineModule.cleanup();
    imageViewModule.cleanup();
    swapchainModule.cleanup();

    bufferModule.cleanupDescriptorBuffer();
    descriptorModule.cleanupDescriptorPool();
}

void App::drawFrame()
{
    synchronizationModule.synchronizeWaitFences();

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(deviceModule->device, swapchainModule.getSwapchain(), UINT64_MAX, synchronizationModule.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);
    resizeSwapchain(result, ERROR_RESIZE::SWAPCHAIN_ERROR);

    synchronizationModule.synchronizeCurrentFrame(imageIndex);

    bufferModule.updateUniformBuffer(imageIndex, swapchainModule.swapChainExtent);

    synchronizationModule.submitCommandBuffer(commandPoolModule->getCommandBuffer(imageIndex), queueModule);

    result = synchronizationModule.presentSwapchain(swapchainModule.getSwapchain(), imageIndex, queueModule);
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
    shaderModule.createShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv", geometryModule);
    depthBufferModule.createDepthResources(swapchainModule.swapChainExtent, queueModule, *commandPoolModule);
    graphicsPipelineModule.addShaderModules(shaderModule);
    graphicsPipelineModule.createRenderPass(swapchainModule.swapChainImageFormat, depthBufferModule);
    graphicsPipelineModule.createGraphicsPipeline(swapchainModule.swapChainExtent, descriptorModule.getDescriptorSetLayout());
    framebufferModule.createFramebuffer(graphicsPipelineModule.renderPass, imageViewModule.swapChainImageViews, swapchainModule.swapChainExtent, depthBufferModule);
    bufferModule.createUniformBuffers(swapchainModule.getNumSwapChainImages());
    descriptorModule.createDescriptorPool(swapchainModule.getNumSwapChainImages());
    descriptorModule.createDescriptorSets();
    commandPoolModule->createCommandBuffers(framebufferModule.swapChainFramebuffers, graphicsPipelineModule.renderPass, swapchainModule.swapChainExtent,
        graphicsPipelineModule.pipelineLayout, graphicsPipelineModule.graphicsPipeline,
        geometryModule, bufferModule, descriptorModule.getDescriptorSet());
}
