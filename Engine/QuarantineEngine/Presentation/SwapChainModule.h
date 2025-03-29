#pragma once
#ifndef SWAPCHAINMODULE_H
#define SWAPCHAINMODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max
#include <GLFW/glfw3.h>

#include "DeviceModule.h"
#include <UBO.h>
#include <QESingleton.h>

class SwapChainModule : public QESingleton<SwapChainModule>
{
private:
    friend class QESingleton<SwapChainModule>; // Permitir acceso al constructor
public:
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::shared_ptr<UniformBufferObject> screenData = nullptr;
    uint32_t  currentImage = 0;

    const uint32_t TILE_SIZE = 8;

private:
    DeviceModule* deviceModule;
    VkSwapchainKHR swapChain;
    uint32_t numSwapChainImages;
    glm::vec2 pixelTileSize;
    float currentTileSize;

public:
    SwapChainModule();
    void createSwapChain(VkSurfaceKHR& surface, GLFWwindow* window);
    void cleanup();
    uint32_t getNumSwapChainImages() { return numSwapChainImages; }
    VkSwapchainKHR &getSwapchain() { return swapChain; }
    void InitializeScreenDataResources();
    void CleanScreenDataResources();
    void UpdateTileSize(float newTileSize);
private:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    void UpdateScreenData();
};

#endif
