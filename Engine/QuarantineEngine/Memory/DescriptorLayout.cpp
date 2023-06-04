#include "DescriptorLayout.h"
#include <SynchronizationModule.h>
#include <Material/Material.h>

DescriptorLayout::DescriptorLayout()
{
    this->deviceModule = DeviceModule::getInstance();
}

void DescriptorLayout::CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> uboLayoutBinding{};
    uboLayoutBinding.resize(this->numUBOs);

    // Inicializamos los descriptor layouts de los UBO's
    for (size_t i = 0; i < this->numUBOs; i++)
    {
        uboLayoutBinding[i].binding = this->numBinding;
        uboLayoutBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding[i].descriptorCount = 1;
        uboLayoutBinding[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboLayoutBinding[i].pImmutableSamplers = nullptr; // Optional
        this->numBinding++;
    }

    // Inicializamos el descriptor layouts de las texturas que será un array texture
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = this->numBinding;
    samplerLayoutBinding.descriptorCount = Material::TOTAL_NUM_TEXTURES;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    this->numBinding++;

    // Formamos el layout total
    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
    bindings.push_back(samplerLayoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DescriptorLayout::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(this->numBinding);

    size_t idx = 0;
    while (idx < this->numUBOs)
    {
        poolSizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        idx++;
    }

    poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(deviceModule->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}
