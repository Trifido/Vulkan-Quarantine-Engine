#pragma once
#ifndef VULKANLAYERANDEXTENSION_H
#define VULKANLAYERANDEXTENSION_H

#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

enum DEBUG_LEVEL { VERBOSE = 0, ONLY_WARNING = 1, ONLY_ERROR = 2, ALL = 3, NOT_VERBOSE = 4 };

struct LayerProperties
{
    VkLayerProperties                   properties;
    std::vector<VkExtensionProperties>  extensions;
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool checkValidationLayerSupport();
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
std::vector<const char*> getRequiredExtensions();
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, DEBUG_LEVEL level);

class VulkanLayerAndExtension
{
public:
    VkDebugUtilsMessengerEXT debugMessenger;
public:
    VkResult getExtensionProperties(LayerProperties& layerProps, VkPhysicalDevice* gpu = NULL);
    void getInstanceExtensions();

    void setupDebugMessenger(VkInstance instance, DEBUG_LEVEL level);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                          const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);
};

#endif
