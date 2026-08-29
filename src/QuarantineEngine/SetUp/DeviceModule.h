#pragma once
#ifndef DEVICEMODULE_H
#define DEVICEMODULE_H

#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

#include "VulkanLayerAndExtension.h"
#include "SwapChainTool.hpp"
#include "QueueModule.h"
#include "QueueFamiliesModule.h"
#include "QESingleton.h"

class DeviceModule : public QESingleton<DeviceModule>
{
private:
    friend class QESingleton<DeviceModule>;

    struct DeviceCapabilities
    {
        uint32_t apiVersion{};
        bool samplerAnisotropy{};
        bool sampleRateShading{};
        bool fillModeNonSolid{};
        bool wideLines{};
        bool sampledImageNonUniformIndexing{};
        bool bufferDeviceAddress{};
        bool extendedDynamicState{};
        bool meshShader{};
        bool taskShader{};
        bool meshShaderExtension{};
    };

    VkSampleCountFlagBits      msaaSamples{ VK_SAMPLE_COUNT_1_BIT };
    VkPhysicalDeviceProperties physicalDeviceProps{};
    DeviceCapabilities         capabilities{};

public:
    VkDevice                            device{ VK_NULL_HANDLE };
    VkPhysicalDevice                    physicalDevice{ VK_NULL_HANDLE };
    QueueFamilyIndices                  queueIndices;

public:
    void pickPhysicalDevice(const VkInstance &instance, VkSurfaceKHR& surface);
    void createLogicalDevice(VkSurfaceKHR& surface, QueueModule& queueModule);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    void cleanup();
    VkSampleCountFlagBits* getMsaaSamples();
    bool supportsBufferDeviceAddress() const noexcept;
    bool supportsMeshShaders() const noexcept;
    bool supportsTaskShaders() const noexcept;
    bool supportsSampleRateShading() const noexcept;
    bool supportsFillModeNonSolid() const noexcept;
    bool supportsWideLines() const noexcept;
private:
    DeviceCapabilities queryDeviceCapabilities(VkPhysicalDevice newDevice) const;
    bool isDeviceSuitable(VkPhysicalDevice newDevice, VkSurfaceKHR& surface, DeviceCapabilities& candidateCapabilities, std::string& rejectionReason) const;
    VkSampleCountFlagBits getMaxUsableSampleCount();
};



namespace QE
{
    using ::DeviceModule;
} // namespace QE
// QE namespace aliases
#endif
