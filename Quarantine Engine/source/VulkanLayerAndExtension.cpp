#pragma once
#include "Headers.h"

VkResult VulkanLayerAndExtension::getInstanceLayerProperties()
{
    uint32_t instanceLayerCount;
    std::vector<VkLayerProperties> layerProperties;
    VkResult result;

    do
    {
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, NULL);

        if(result)
            return result;
        if (instanceLayerCount ==  0)
            return VK_INCOMPLETE;

        layerProperties.resize(instanceLayerCount);
        result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, layerProperties.data());
    } while (result == VK_INCOMPLETE);

    std::cout << "\nInstance Layers" << std::endl;
    std::cout << "=================" << std::endl;
    for (auto globalLayerProp: layerProperties)
    {
        std::cout << "\n" << globalLayerProp.description << 
        "\n\t|\n\t|---[Layer Name]--> " << globalLayerProp.layerName << "\n";

        LayerProperties layerProps;
        layerProps.properties = globalLayerProp;

        result = getExtensionProperties(layerProps);

        if(result)
            continue;
        
        for(auto j : layerProps.extensions)
        {
            std::cout << "\t\t|\n\t\t|---[Layer Extension]--> " 
                      << j.extensionName << "\n";
        }
    }
    return result;
}

VkResult VulkanLayerAndExtension::getExtensionProperties(LayerProperties &layerProps, VkPhysicalDevice* gpu)
{
    uint32_t extensionCount;
    VkResult result;
    char* layerName = layerProps.properties.layerName;

    do
    {
        if(gpu)
            result = vkEnumerateDeviceExtensionProperties(*gpu, layerName, &extensionCount, NULL);
        else
            result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, NULL);

        if(result || extensionCount == 0)
            continue;

        if(gpu)
            result = vkEnumerateDeviceExtensionProperties(*gpu, layerName, &extensionCount, layerProps.extensions.data());
        else
            result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, layerProps.extensions.data());
    }while( result == VK_INCOMPLETE);
}