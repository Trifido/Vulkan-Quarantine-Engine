#pragma once
#ifndef VULKANLAYERANDEXTENSION_H
#define VULKANLAYERANDEXTENSION_H

#include <vector>
#include <iostream>
#include <vulkan/vulkan.hpp>
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
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor"
};

const std::vector<const char*> instanceExtensions = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME,
    "VK_KHR_win32_surface",
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
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
