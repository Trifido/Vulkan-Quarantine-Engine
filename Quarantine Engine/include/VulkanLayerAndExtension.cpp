struct LayerProperties
{
    VkLayerProperties properties;
    vector<VkExtensionProperties> extensions;
};

class VulkanLayerAndExtension
{
    std::vector<LayerProperties> getInstanceLayerProperties();

    VkResult getExtensionProperties(LayerProperties &layerProps, VkPhysicalDevice* gpu = NULL);
    VkResult getDeviceExtensionProperties(VkPhysicalDevice* gpu);
}