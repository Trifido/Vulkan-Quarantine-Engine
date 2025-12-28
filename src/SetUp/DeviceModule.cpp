#include "DeviceModule.h"
#include <stdexcept>
#include <map>
#include <set>
#include "QueueFamiliesModule.h"

VkSampleCountFlagBits DeviceModule::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

void DeviceModule::pickPhysicalDevice(const VkInstance &newInstance, VkSurfaceKHR& surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(newInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        std::cout << "failed to find GPUs with Vulkan support!\n";
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(newInstance, &deviceCount, devices.data());

    for (const auto& nDevice : devices)
    {
        if (isDeviceSuitable(nDevice, surface))
        {
            physicalDevice = nDevice;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        std::cout << "failed to find a suitable GPU!\n";
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    this->InitializeMeshShaderExtension();
}

void DeviceModule::createLogicalDevice(VkSurfaceKHR& surface, QueueModule& nQueueModule)
{
    // --- Queues ---
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies =
    {
        indices.graphicsFamily.value(),
        indices.presentFamily.value(),
        indices.computeFamily.value()
    };
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = queueFamily;
        q.queueCount = 1;
        q.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(q);
    }

    // --- FEATURES ---
    // Core features2
    VkPhysicalDeviceFeatures2 feats2{};
    feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Descriptor indexing (bindless)
    VkPhysicalDeviceDescriptorIndexingFeatures indexing{};
    indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

    // Maintenance4
    VkPhysicalDeviceMaintenance4Features maintenance4{};
    maintenance4.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;

    // 8-bit storage
    VkPhysicalDevice8BitStorageFeatures storage8{};
    storage8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;

    // Mesh shaders (EXT)
    VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsr = {};
    fsr.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
    fsr.primitiveFragmentShadingRate = VK_TRUE;
    fsr.attachmentFragmentShadingRate = VK_TRUE;
    fsr.pipelineFragmentShadingRate = VK_TRUE;

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh = {};
    mesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    mesh.pNext = &fsr;
    mesh.meshShader = VK_TRUE;
    mesh.taskShader = VK_TRUE;
    mesh.primitiveFragmentShadingRateMeshShader = VK_TRUE;

    // Buffer Device Address (KHR/core)
    VkPhysicalDeviceBufferDeviceAddressFeatures bda{};
    bda.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

    feats2.pNext = &bda;
    bda.pNext = &mesh;
    mesh.pNext = &storage8;
    storage8.pNext = &maintenance4;
    maintenance4.pNext = &indexing;
    indexing.pNext = nullptr;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &feats2);

    // Core features
    VkPhysicalDeviceFeatures core{};
    core.samplerAnisotropy = feats2.features.samplerAnisotropy;
    core.sampleRateShading = feats2.features.sampleRateShading;
    core.fillModeNonSolid = feats2.features.fillModeNonSolid;
    core.wideLines = feats2.features.wideLines;
    core.vertexPipelineStoresAndAtomics = true;

    // Optional Bindless opcional 
    if (this->bindless_supported)
    {
        indexing.descriptorBindingPartiallyBound = indexing.descriptorBindingPartiallyBound ? VK_TRUE : VK_FALSE;
        indexing.runtimeDescriptorArray = indexing.runtimeDescriptorArray ? VK_TRUE : VK_FALSE;
    }
    else
    {
        indexing = {};
        indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    }

    // Maintenance4
    maintenance4.maintenance4 = maintenance4.maintenance4 ? VK_TRUE : VK_FALSE;

    // 8-bit storage
    storage8.storageBuffer8BitAccess = storage8.storageBuffer8BitAccess ? VK_TRUE : VK_FALSE;
    storage8.storagePushConstant8 = storage8.storagePushConstant8 ? VK_TRUE : VK_FALSE;

    /*
    if (GetModuleHandleA("renderdoc.dll") != nullptr) {
        mesh.taskShader = VK_FALSE;
        mesh.meshShader = VK_FALSE;
    } else {
        mesh.taskShader = mesh.taskShader ? VK_TRUE : VK_FALSE;
        mesh.meshShader = mesh.meshShader ? VK_TRUE : VK_FALSE;
    }
    */

    mesh.taskShader = mesh.taskShader ? VK_TRUE : VK_FALSE;
    mesh.meshShader = mesh.meshShader ? VK_TRUE : VK_FALSE;

    // Buffer Device Address (KHR/core)
    bda.bufferDeviceAddress = bda.bufferDeviceAddress ? VK_TRUE : VK_FALSE;


    std::vector<const char*> enabledExtensions = deviceExtensions;
    auto removeExt = [&](const char* name) {
        enabledExtensions.erase(
            std::remove_if(enabledExtensions.begin(), enabledExtensions.end(),
                [&](const char* s) { return std::strcmp(s, name) == 0; }),
            enabledExtensions.end()
        );
    };

    if (!mesh.meshShader || !mesh.taskShader)
    {
        removeExt(VK_EXT_MESH_SHADER_EXTENSION_NAME);
    }
    if (!bda.bufferDeviceAddress)
    {
        removeExt(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    }

    feats2.features = core;
    feats2.pNext = &bda;
    bda.pNext = &mesh;
    mesh.pNext = &fsr;
    fsr.pNext = &storage8;
    storage8.pNext = &maintenance4;
    maintenance4.pNext = &indexing;
    indexing.pNext = nullptr;

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    ci.pQueueCreateInfos = queueCreateInfos.data();
    ci.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    ci.ppEnabledExtensionNames = enabledExtensions.data();

    ci.pNext = &feats2;
    ci.pEnabledFeatures = nullptr;

    if (enableValidationLayers)
    {
        ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ci.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        ci.enabledLayerCount = 0;
    }

    VkResult r = vkCreateDevice(physicalDevice, &ci, nullptr, &device);
    if (r != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    // --- Result Queues ---
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &nQueueModule.graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &nQueueModule.presentQueue);
    vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &nQueueModule.computeQueue);
}


void DeviceModule::cleanup()
{
    vkDestroyDevice(device, nullptr);
    this->ResetInstance();
}

VkFormat DeviceModule::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }

        //throw std::runtime_error("failed to find supported format!");
    }

    return VkFormat::VK_FORMAT_UNDEFINED;
}

VkSampleCountFlagBits* DeviceModule::getMsaaSamples()
{
    return &msaaSamples;
}

void DeviceModule::InitializeMeshShaderExtension()
{
    VkPhysicalDeviceMeshShaderPropertiesNV meshShaderProperties = {};
    meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;

    VkPhysicalDeviceProperties2 props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &meshShaderProperties;

    vkGetPhysicalDeviceProperties2(this->physicalDevice, &props);
}

bool DeviceModule::isDeviceSuitable(VkPhysicalDevice newDevice, VkSurfaceKHR& surface) {
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(newDevice, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(newDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = SwapChainSupportDetails::querySwapChainSupport(newDevice, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(newDevice, &supportedFeatures);

    this->indexing_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, nullptr };
    VkPhysicalDeviceFeatures2 device_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features };

    vkGetPhysicalDeviceFeatures2(newDevice, &device_features);
    this->bindless_supported = this->indexing_features.descriptorBindingPartiallyBound && indexing_features.runtimeDescriptorArray;

    return indices.isComplete() && extensionsSupported
        && swapChainAdequate && supportedFeatures.samplerAnisotropy
        && this->bindless_supported;
}
