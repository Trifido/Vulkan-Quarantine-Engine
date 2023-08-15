#include "ShaderModule.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

ShaderModule::ShaderModule()
{
    deviceModule = DeviceModule::getInstance();
    graphicsPipelineManager = GraphicsPipelineManager::getInstance();
    this->reflectShader = ReflectShader();
}

ShaderModule::ShaderModule(std::string computeShaderName) : ShaderModule()
{
    this->createShaderModule(computeShaderName);
}

ShaderModule::ShaderModule(std::string vertexShaderName, std::string fragmentShaderName) : ShaderModule()
{
    this->createShaderModule(vertexShaderName, fragmentShaderName);
}

std::vector<char> ShaderModule::readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

void ShaderModule::createShaderModule(const std::string& filename_compute)
{
    compShaderStageInfo = createShader(deviceModule->device, filename_compute, SHADER_TYPE::COMPUTE_SHADER);
    shaderStages.push_back(compShaderStageInfo);

    this->CreateDescriptorSetLayout();
}

void ShaderModule::createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment)
{
    vertShaderStageInfo = createShader(deviceModule->device, filename_vertex, SHADER_TYPE::VERTEX_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    this->CreateDescriptorSetLayout();
    this->createShaderBindings();
    this->PipelineModule = this->graphicsPipelineManager->RegisterNewGraphicsPipeline(*this, descriptorSetLayout);
}

void ShaderModule::CleanDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(deviceModule->device, this->descriptorSetLayout, nullptr);
}

void ShaderModule::createShaderBindings()
{
    this->bindingDescription = GeometryComponent::getBindingDescription(this->reflectShader.isAnimationShader);
    this->attributeDescriptions = GeometryComponent::getAttributeDescriptions(this->reflectShader.isAnimationShader);

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &this->bindingDescription; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(this->attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = this->attributeDescriptions.data(); // Optional
}

void ShaderModule::cleanup()
{
    shaderStages.clear();
    if (vertex_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, vertex_shader, nullptr);
    if(fragment_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, fragment_shader, nullptr);
    if (compute_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, compute_shader, nullptr);
}

VkPipelineShaderStageCreateInfo ShaderModule::createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType)
{
    std::vector<char> code = readFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    //this->reflectShader.Output(createInfo);
    this->reflectShader.PerformReflect(createInfo);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    switch (shaderType)
    {
        case SHADER_TYPE::VERTEX_SHADER:

            if (vkCreateShaderModule(device, &createInfo, nullptr, &vertex_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create vertex shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStageInfo.module = vertex_shader;
            break;

        case SHADER_TYPE::FRAGMENT_SHADER:

            if (vkCreateShaderModule(device, &createInfo, nullptr, &fragment_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create fragment shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStageInfo.module = fragment_shader;
            break;

        case SHADER_TYPE::GEOMETRY_SHADER:
            break;
        case SHADER_TYPE::TESELLATION_SHADER:
            break;
        case SHADER_TYPE::COMPUTE_SHADER:
            if (vkCreateShaderModule(device, &createInfo, nullptr, &compute_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderStageInfo.module = compute_shader;
            break;
        default:
            break;
    }
    shaderStageInfo.pName = "main";

    return shaderStageInfo;
}

void ShaderModule::CreateDescriptorSetLayout()
{
    //this->reflectShader.CheckMixStageBindings();

    std::vector<VkDescriptorSetLayoutBinding> bindings{};

    for each (auto reflectLayotBinding in this->reflectShader.bindings)
    {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = reflectLayotBinding.second.binding;
        layoutBinding.descriptorType = reflectLayotBinding.second.type;
        layoutBinding.descriptorCount = reflectLayotBinding.second.arraySize;
        layoutBinding.stageFlags = reflectLayotBinding.second.stage;
        layoutBinding.pImmutableSamplers = nullptr;
        bindings.push_back(layoutBinding);
    }

    //for each (auto reflectLayotBinding in this->reflectShader.mixBindings)
    //{
    //    VkDescriptorSetLayoutBinding layoutBinding = {};
    //    layoutBinding.binding = reflectLayotBinding.second.binding;
    //    layoutBinding.descriptorType = reflectLayotBinding.second.type;
    //    layoutBinding.descriptorCount = reflectLayotBinding.second.arraySize;
    //    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    //    layoutBinding.pImmutableSamplers = nullptr;
    //    bindings.push_back(layoutBinding);
    //}

    //for each (auto reflectLayotBinding in this->reflectShader.descriptorSetReflect)
    //{
    //    for (int id = 0; id < reflectLayotBinding.bindingCount; id++)
    //    {
    //        if (!reflectShader.IsMixBinding(reflectLayotBinding.bindings[id].name))
    //        {
    //            VkDescriptorSetLayoutBinding layoutBinding = {};
    //            layoutBinding.binding = reflectLayotBinding.bindings[id].binding;
    //            layoutBinding.descriptorType = reflectLayotBinding.bindings[id].type;
    //            layoutBinding.descriptorCount = reflectLayotBinding.bindings[id].arraySize;
    //            layoutBinding.stageFlags = reflectLayotBinding.stage;
    //            layoutBinding.pImmutableSamplers = nullptr;
    //            bindings.push_back(layoutBinding);
    //        }
    //    }
    //}

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &this->descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}
