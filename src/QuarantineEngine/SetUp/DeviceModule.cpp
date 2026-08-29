#include "DeviceModule.h"

#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "QueueFamiliesModule.h"
#include <Logging/QELogMacros.h>

namespace
{
    constexpr uint32_t MinimumVulkanApiVersion = VK_API_VERSION_1_3;

    std::unordered_set<std::string> EnumerateDeviceExtensions(VkPhysicalDevice device)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::unordered_set<std::string> result;
        result.reserve(availableExtensions.size());
        for (const auto& extension : availableExtensions)
        {
            result.emplace(extension.extensionName);
        }
        return result;
    }

    std::string FormatApiVersion(uint32_t version)
    {
        return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
            std::to_string(VK_API_VERSION_MINOR(version)) + "." +
            std::to_string(VK_API_VERSION_PATCH(version));
    }

    int ScoreDevice(const VkPhysicalDeviceProperties& properties)
    {
        int score = static_cast<int>(properties.limits.maxImageDimension2D);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 100000;
        }
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            score += 50000;
        }
        return score;
    }
}

VkSampleCountFlagBits DeviceModule::getMaxUsableSampleCount()
{
    const VkSampleCountFlags counts = physicalDeviceProps.limits.framebufferColorSampleCounts &
        physicalDeviceProps.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

DeviceModule::DeviceCapabilities DeviceModule::queryDeviceCapabilities(VkPhysicalDevice newDevice) const
{
    DeviceCapabilities result{};

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(newDevice, &properties);
    result.apiVersion = properties.apiVersion;

    if (properties.apiVersion < MinimumVulkanApiVersion)
    {
        return result;
    }

    const auto extensions = EnumerateDeviceExtensions(newDevice);
    result.meshShaderExtension = extensions.contains(VK_EXT_MESH_SHADER_EXTENSION_NAME);

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
    descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress{};
    bufferDeviceAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicState{};
    extendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShader{};
    meshShader.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

    features.pNext = &descriptorIndexing;
    descriptorIndexing.pNext = &bufferDeviceAddress;
    bufferDeviceAddress.pNext = &extendedDynamicState;
    extendedDynamicState.pNext = result.meshShaderExtension ? &meshShader : nullptr;

    vkGetPhysicalDeviceFeatures2(newDevice, &features);

    result.samplerAnisotropy = features.features.samplerAnisotropy == VK_TRUE;
    result.sampleRateShading = features.features.sampleRateShading == VK_TRUE;
    result.fillModeNonSolid = features.features.fillModeNonSolid == VK_TRUE;
    result.wideLines = features.features.wideLines == VK_TRUE;
    result.sampledImageNonUniformIndexing =
        descriptorIndexing.shaderSampledImageArrayNonUniformIndexing == VK_TRUE;
    result.bufferDeviceAddress = bufferDeviceAddress.bufferDeviceAddress == VK_TRUE;
    result.extendedDynamicState = extendedDynamicState.extendedDynamicState == VK_TRUE;
    result.meshShader = result.meshShaderExtension && meshShader.meshShader == VK_TRUE;
    result.taskShader = result.meshShader && meshShader.taskShader == VK_TRUE;

    return result;
}

bool DeviceModule::isDeviceSuitable(
    VkPhysicalDevice newDevice,
    VkSurfaceKHR& surface,
    DeviceCapabilities& candidateCapabilities,
    std::string& rejectionReason) const
{
    const QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(newDevice, surface);
    if (!indices.isComplete())
    {
        rejectionReason = "missing graphics, compute or presentation queue";
        return false;
    }

    if (!checkDeviceExtensionSupport(newDevice))
    {
        rejectionReason = "missing required device extension: VK_KHR_swapchain";
        return false;
    }

    const SwapChainSupportDetails swapChainSupport =
        SwapChainSupportDetails::querySwapChainSupport(newDevice, surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
    {
        rejectionReason = "surface has no compatible swapchain format or presentation mode";
        return false;
    }

    candidateCapabilities = queryDeviceCapabilities(newDevice);
    if (candidateCapabilities.apiVersion < MinimumVulkanApiVersion)
    {
        rejectionReason = "Vulkan " + FormatApiVersion(candidateCapabilities.apiVersion) +
            " is below the required Vulkan 1.3 profile";
        return false;
    }
    if (!candidateCapabilities.sampledImageNonUniformIndexing)
    {
        rejectionReason = "shaderSampledImageArrayNonUniformIndexing is not supported";
        return false;
    }
    if (!candidateCapabilities.extendedDynamicState)
    {
        rejectionReason = "extendedDynamicState is not supported";
        return false;
    }

    rejectionReason.clear();
    return true;
}

void DeviceModule::pickPhysicalDevice(const VkInstance& newInstance, VkSurfaceKHR& surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(newInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        QE_LOG_ERROR_CAT("Vulkan", "No GPUs with Vulkan support found");
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(newInstance, &deviceCount, devices.data());

    int bestScore = -1;
    for (const VkPhysicalDevice candidate : devices)
    {
        VkPhysicalDeviceProperties candidateProperties{};
        vkGetPhysicalDeviceProperties(candidate, &candidateProperties);

        DeviceCapabilities candidateCapabilities{};
        std::string rejectionReason;
        if (!isDeviceSuitable(candidate, surface, candidateCapabilities, rejectionReason))
        {
            QE_LOG_WARN_CAT_F(
                "Vulkan",
                "Rejected GPU '{}': {}",
                candidateProperties.deviceName,
                rejectionReason);
            continue;
        }

        const int score = ScoreDevice(candidateProperties);
        QE_LOG_INFO_CAT_F(
            "Vulkan",
            "Compatible GPU '{}' (Vulkan {}, score {})",
            candidateProperties.deviceName,
            FormatApiVersion(candidateProperties.apiVersion),
            score);

        if (score > bestScore)
        {
            bestScore = score;
            physicalDevice = candidate;
            physicalDeviceProps = candidateProperties;
            capabilities = candidateCapabilities;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        QE_LOG_ERROR_CAT("Vulkan", "Failed to find a GPU compatible with the stable Vulkan profile");
        throw std::runtime_error("failed to find a GPU compatible with the stable Vulkan profile");
    }

    queueIndices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    msaaSamples = getMaxUsableSampleCount();

    QE_LOG_INFO_CAT_F(
        "Vulkan",
        "Selected GPU '{}' | Vulkan {} | vendor 0x{:04X} device 0x{:04X} | driver {}",
        physicalDeviceProps.deviceName,
        FormatApiVersion(physicalDeviceProps.apiVersion),
        physicalDeviceProps.vendorID,
        physicalDeviceProps.deviceID,
        physicalDeviceProps.driverVersion);
    QE_LOG_INFO_CAT_F(
        "Vulkan",
        "Capabilities: BDA={} non-uniform textures={} dynamic-state={} mesh={} task={} sample-shading={} wireframe={} wide-lines={}",
        capabilities.bufferDeviceAddress,
        capabilities.sampledImageNonUniformIndexing,
        capabilities.extendedDynamicState,
        capabilities.meshShader,
        capabilities.taskShader,
        capabilities.sampleRateShading,
        capabilities.fillModeNonSolid,
        capabilities.wideLines);
}

void DeviceModule::createLogicalDevice(VkSurfaceKHR& surface, QueueModule& nQueueModule)
{
    if (physicalDevice == VK_NULL_HANDLE || !queueIndices.isComplete())
    {
        throw std::runtime_error("Cannot create a logical device before selecting a compatible physical device");
    }

    const QueueFamilyIndices currentIndices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    if (!currentIndices.isComplete())
    {
        throw std::runtime_error("Selected GPU no longer exposes all required queue families");
    }
    queueIndices = currentIndices;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const std::set<uint32_t> uniqueQueueFamilies =
    {
        queueIndices.graphicsFamily.value(),
        queueIndices.presentFamily.value(),
        queueIndices.computeFamily.value()
    };
    float queuePriority = 1.0f;

    for (const uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures2 enabledFeatures{};
    enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabledFeatures.features.samplerAnisotropy = capabilities.samplerAnisotropy ? VK_TRUE : VK_FALSE;
    enabledFeatures.features.sampleRateShading = capabilities.sampleRateShading ? VK_TRUE : VK_FALSE;
    enabledFeatures.features.fillModeNonSolid = capabilities.fillModeNonSolid ? VK_TRUE : VK_FALSE;
    enabledFeatures.features.wideLines = capabilities.wideLines ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexing{};
    descriptorIndexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptorIndexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress{};
    bufferDeviceAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddress.bufferDeviceAddress = capabilities.bufferDeviceAddress ? VK_TRUE : VK_FALSE;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicState{};
    extendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicState.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShader{};
    meshShader.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    meshShader.meshShader = capabilities.meshShader ? VK_TRUE : VK_FALSE;
    meshShader.taskShader = capabilities.taskShader ? VK_TRUE : VK_FALSE;

    enabledFeatures.pNext = &descriptorIndexing;
    descriptorIndexing.pNext = &bufferDeviceAddress;
    bufferDeviceAddress.pNext = &extendedDynamicState;
    extendedDynamicState.pNext = capabilities.meshShader ? &meshShader : nullptr;

    std::vector<const char*> enabledExtensions = requiredDeviceExtensions;
    if (capabilities.meshShader)
    {
        enabledExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.pNext = &enabledFeatures;
    createInfo.pEnabledFeatures = nullptr;

    const VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (result != VK_SUCCESS)
    {
        QE_LOG_ERROR_CAT_F("Vulkan", "vkCreateDevice failed with VkResult {}", static_cast<int>(result));
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device, queueIndices.graphicsFamily.value(), 0, &nQueueModule.graphicsQueue);
    vkGetDeviceQueue(device, queueIndices.presentFamily.value(), 0, &nQueueModule.presentQueue);
    vkGetDeviceQueue(device, queueIndices.computeFamily.value(), 0, &nQueueModule.computeQueue);
}

void DeviceModule::cleanup()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    physicalDevice = VK_NULL_HANDLE;
    physicalDeviceProps = {};
    capabilities = {};
    queueIndices = {};
    msaaSamples = VK_SAMPLE_COUNT_1_BIT;
}

VkFormat DeviceModule::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features)
{
    for (const VkFormat format : candidates)
    {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (properties.linearTilingFeatures & features) == features)
        {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL &&
            (properties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkSampleCountFlagBits* DeviceModule::getMsaaSamples()
{
    return &msaaSamples;
}

bool DeviceModule::supportsBufferDeviceAddress() const noexcept
{
    return capabilities.bufferDeviceAddress;
}

bool DeviceModule::supportsMeshShaders() const noexcept
{
    return capabilities.meshShader;
}

bool DeviceModule::supportsTaskShaders() const noexcept
{
    return capabilities.taskShader;
}

bool DeviceModule::supportsSampleRateShading() const noexcept
{
    return capabilities.sampleRateShading;
}

bool DeviceModule::supportsFillModeNonSolid() const noexcept
{
    return capabilities.fillModeNonSolid;
}

bool DeviceModule::supportsWideLines() const noexcept
{
    return capabilities.wideLines;
}
