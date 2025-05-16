#pragma once
#ifndef DEVICEMODULE_H
#define DEVICEMODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>

#include "VulkanLayerAndExtension.h"
#include "SwapChainTool.hpp"
#include "QueueModule.h"
#include "QueueFamiliesModule.h"
#include "QESingleton.h"

class DeviceModule : public QESingleton<DeviceModule>
{
private:
    friend class QESingleton<DeviceModule>; // Permitir acceso al constructor
    VkSampleCountFlagBits               msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceProperties          physicalDeviceProps;
    VkPhysicalDeviceFeatures            physicalDeviceFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
    VkPhysicalDeviceMemoryProperties    memoryProperties;
    QueueModule                         queueModule;
    bool                                bindless_supported;
    bool                                meshShader_supported;

public:
    VkDevice                            device;
    VkPhysicalDevice                    physicalDevice;
    QueueFamilyIndices                  queueIndices;

public:
    void pickPhysicalDevice(const VkInstance &instance, VkSurfaceKHR& surface);
    void createLogicalDevice(VkSurfaceKHR& surface, QueueModule& queueModule);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    void cleanup();
    VkSampleCountFlagBits* getMsaaSamples();
    void InitializeMeshShaderExtension();
private:
    bool isDeviceSuitable(VkPhysicalDevice newDevice, VkSurfaceKHR& surface);
    VkSampleCountFlagBits getMaxUsableSampleCount();
};

#endif
