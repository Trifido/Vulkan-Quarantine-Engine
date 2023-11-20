#pragma once
#ifndef DEVICEMODULE_H
#define DEVICEMODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>

#include "VulkanLayerAndExtension.h"
#include "SwapChainTool.hpp"
#include "QueueModule.h"
#include "QueueFamiliesModule.h"

class DeviceModule
{
public:
    VkDevice                            device;
    VkPhysicalDevice                    physicalDevice;
    QueueFamilyIndices                  queueIndices;
private:
    static DeviceModule*                instance;
    VkSampleCountFlagBits               msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceProperties          physicalDeviceProps;
    VkPhysicalDeviceFeatures            physicalDeviceFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
    VkPhysicalDeviceMemoryProperties    memoryProperties;
    QueueModule                         queueModule;
    bool                                bindless_supported;
    bool                                meshShader_supported;

public:
    static DeviceModule* getInstance();
    static void ResetInstance();

    void pickPhysicalDevice(const VkInstance &instance, VkSurfaceKHR& surface);
    void createLogicalDevice(VkSurfaceKHR& surface, QueueModule& queueModule);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    void cleanup();
    VkSampleCountFlagBits* getMsaaSamples();
private:
    bool isDeviceSuitable(VkPhysicalDevice newDevice, VkSurfaceKHR& surface);
    VkSampleCountFlagBits getMaxUsableSampleCount();
};

#endif
