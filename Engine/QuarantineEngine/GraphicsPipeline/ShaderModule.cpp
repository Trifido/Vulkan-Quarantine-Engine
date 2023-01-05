#include "ShaderModule.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

ShaderModule::ShaderModule()
{
    deviceModule = DeviceModule::getInstance();
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
}

void ShaderModule::createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment)
{
    vertShaderStageInfo = createShader(deviceModule->device, filename_vertex, SHADER_TYPE::VERTEX_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);
}

void ShaderModule::createShaderBindings(bool hasAnimation)
{
    this->bindingDescription = GeometryComponent::getBindingDescription(hasAnimation);
    this->attributeDescriptions = GeometryComponent::getAttributeDescriptions(hasAnimation);

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
