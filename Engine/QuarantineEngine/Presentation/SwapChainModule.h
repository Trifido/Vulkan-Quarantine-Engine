#pragma once
#ifndef SWAPCHAINMODULE_H
#define SWAPCHAINMODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max
#include <GLFW/glfw3.h>

#include "DeviceModule.h"

class SwapChainModule
{
public:
    static SwapChainModule* instance;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
private:
    DeviceModule* deviceModule;
    VkSwapchainKHR swapChain;
    uint32_t numSwapChainImages;

public:
    static SwapChainModule* getInstance();
    static void ResetInstance();
    SwapChainModule();
    void createSwapChain(VkSurfaceKHR& surface, GLFWwindow* window);
    void cleanup();
    uint32_t getNumSwapChainImages() { return numSwapChainImages; }
    VkSwapchainKHR &getSwapchain() { return swapChain; }
private:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
};

#endif
