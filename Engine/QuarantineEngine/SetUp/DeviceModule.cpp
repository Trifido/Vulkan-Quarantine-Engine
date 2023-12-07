#include "DeviceModule.h"
#include <stdexcept>
#include <map>
#include <set>
#include "QueueFamiliesModule.h"

DeviceModule* DeviceModule::instance = nullptr;

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

    if (deviceCount == 0) {
        std::cout << "failed to find GPUs with Vulkan support!\n";
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(newInstance, &deviceCount, devices.data());

    for (const auto& nDevice : devices) {
        if (isDeviceSuitable(nDevice, surface)) {
            physicalDevice = nDevice;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        std::cout << "failed to find a suitable GPU!\n";
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    this->InitializeMeshShaderExtension();
}

DeviceModule* DeviceModule::getInstance()
{
    if (instance == nullptr)
        instance = new DeviceModule();

    return instance;
}

void DeviceModule::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

void DeviceModule::createLogicalDevice(VkSurfaceKHR &surface, QueueModule& nQueueModule)
{
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.computeFamily.value() };
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    physicalDeviceFeatures.sampleRateShading = VK_TRUE;
    physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

    VkPhysicalDeviceMaintenance4Features maintenance4Feature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES };
    maintenance4Feature.maintenance4 = VK_TRUE;
    maintenance4Feature.pNext = &indexing_features;

    //8 bit storage features
    VkPhysicalDevice8BitStorageFeatures device8BitFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES };
    device8BitFeature.storageBuffer8BitAccess = VK_TRUE;
    device8BitFeature.storagePushConstant8 = VK_TRUE;
    device8BitFeature.pNext = &maintenance4Feature;

    //Bindless features
    VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    physical_features2.features.samplerAnisotropy = VK_TRUE;
    physical_features2.features.sampleRateShading = VK_TRUE;
    physical_features2.features.fillModeNonSolid = VK_TRUE;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &physical_features2);

    if (this->bindless_supported) {
        this->indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
        this->indexing_features.runtimeDescriptorArray = VK_TRUE;

        physical_features2.pNext = &device8BitFeature;
    }
    //Mesh shader features
    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shaders_feature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    mesh_shaders_feature.taskShader = VK_TRUE;
    mesh_shaders_feature.meshShader = VK_TRUE;
    mesh_shaders_feature.pNext = &physical_features2;

    //Raytracing features
    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
    bufferDeviceAddressFeatures.pNext = &mesh_shaders_feature;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
    bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.pNext = &bufferDeviceAddressFeatures;
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    accelerationStructureFeatures.accelerationStructureCaptureReplay = VK_TRUE;
    accelerationStructureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
    accelerationStructureFeatures.accelerationStructureHostCommands = VK_FALSE;
    accelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;
    

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &accelerationStructureFeatures;

    createInfo.pEnabledFeatures = NULL;// &physicalDeviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &nQueueModule.graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &nQueueModule.presentQueue);
    vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &nQueueModule.computeQueue);
}

void DeviceModule::cleanup()
{
    vkDestroyDevice(device, nullptr);

    if (instance != NULL)
    {
        delete instance;
        instance = NULL;
    }
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
