#include "SwapChainModule.h"
#include "QueueFamiliesModule.h"
#include <stdexcept>

#include "ImageMemoryTools.h"
#include "SwapChainTool.hpp"
#include <SynchronizationModule.h>

SwapChainModule* SwapChainModule::instance = nullptr;

SwapChainModule* SwapChainModule::getInstance()
{
    if (instance == NULL)
        instance = new SwapChainModule();

    return instance;
}

void SwapChainModule::ResetInstance()
{
	delete instance;
	instance = nullptr;
}

SwapChainModule::SwapChainModule()
{
    deviceModule = DeviceModule::getInstance();
    this->currentTileSize = this->TILE_SIZE;
}

void SwapChainModule::createSwapChain(VkSurfaceKHR& surface, GLFWwindow* window)
{
    SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(deviceModule->physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    numSwapChainImages = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && numSwapChainImages > swapChainSupport.capabilities.maxImageCount) {
        numSwapChainImages = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = numSwapChainImages;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(deviceModule->physicalDevice, surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.computeFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 3;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(deviceModule->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(deviceModule->device, swapChain, &numSwapChainImages, nullptr);
    swapChainImages.resize(numSwapChainImages);
    vkGetSwapchainImagesKHR(deviceModule->device, swapChain, &numSwapChainImages, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    swapChainImageViews.resize(this->numSwapChainImages);

    for (size_t i = 0; i < this->numSwapChainImages; i++)
    {
        swapChainImageViews[i] = IMT::createImageView(deviceModule->device, this->swapChainImages[i], this->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    this->UpdateScreenData();
}

void SwapChainModule::cleanup()
{
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(deviceModule->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(deviceModule->device, swapChain, nullptr);
}

VkSurfaceFormatKHR SwapChainModule::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChainModule::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainModule::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void SwapChainModule::UpdateScreenData()
{
    this->pixelTileSize = { this->swapChainExtent.width / this->currentTileSize, this->swapChainExtent.height / this->currentTileSize };
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->screenData->uniformBuffersMemory[currentFrame], 0, sizeof(glm::vec2), 0, &data);
        memcpy(data, &this->pixelTileSize, sizeof(glm::vec2));
        vkUnmapMemory(deviceModule->device, this->screenData->uniformBuffersMemory[currentFrame]);
    }
}

void SwapChainModule::InitializeScreenDataResources()
{
    this->screenData = std::make_shared<UniformBufferObject>();
    this->screenData->CreateUniformBuffer(sizeof(glm::vec2), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void SwapChainModule::CleanScreenDataResources()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->screenData != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->screenData->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->screenData->uniformBuffersMemory[i], nullptr);
        }
    }
}

void SwapChainModule::UpdateTileSize(float newTileSize)
{
    this->currentTileSize = newTileSize;
}
