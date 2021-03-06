#pragma once
#ifndef DEVICEMODULE_H
#define DEVICEMODULE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanLayerAndExtension.h"
#include "SwapChainTool.hpp"
#include "QueueModule.h"

class DeviceModule
{
public:
    VkDevice                            device;
    VkPhysicalDevice                    physicalDevice;
private:
    static DeviceModule*                instance;
    VkSampleCountFlagBits               msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceProperties          physicalDeviceProps;
    VkPhysicalDeviceFeatures            physicalDeviceFeatures{};
    VkPhysicalDeviceMemoryProperties    memoryProperties;
    QueueModule                         queueModule;

public:
    static DeviceModule* getInstance();
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
