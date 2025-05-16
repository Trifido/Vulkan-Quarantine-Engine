#include "ShaderModule.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

ShaderModule::ShaderModule(std::string shaderId)
{
    this->shaderNameID = shaderId;
    this->deviceModule = DeviceModule::getInstance();
    this->graphicsPipelineManager = GraphicsPipelineManager::getInstance();
    this->shadowPipelineManager = ShadowPipelineManager::getInstance();
    this->computePipelineManager = ComputePipelineManager::getInstance();
    this->reflectShader = ReflectShader();
}

ShaderModule::ShaderModule(std::string shaderId, std::string shaderName, GraphicsPipelineData pipelineData) : ShaderModule(shaderId)
{
    this->graphicsPipelineData = pipelineData;

    if (this->graphicsPipelineData.shadowMode != ShadowMappingMode::NONE)
    {
        this->createShadowShaderModule(shaderName);
    }
    else
    {
        this->createShaderModule(shaderName);
    }
}

ShaderModule::ShaderModule(std::string shaderId, std::string vertexShaderName, std::string fragmentShaderName, GraphicsPipelineData pipelineData) : ShaderModule(shaderId)
{
    this->graphicsPipelineData = pipelineData;

    if (this->graphicsPipelineData.shadowMode != ShadowMappingMode::NONE)
    {
        this->createShadowShaderModule(vertexShaderName, fragmentShaderName);
    }
    else
    {
        this->createShaderModule(vertexShaderName, fragmentShaderName);
    }
}

ShaderModule::ShaderModule(std::string shaderId, std::string firstShaderName, std::string secondShaderName, std::string thirdShaderName, GraphicsPipelineData pipelineData)
    : ShaderModule(shaderId)
{
    this->graphicsPipelineData = pipelineData;

    if (!this->graphicsPipelineData.IsMeshShader)
    {
        this->createShaderModule(firstShaderName, secondShaderName, thirdShaderName);
    }
    else
    {
        this->createMeshShaderModule(firstShaderName, secondShaderName, thirdShaderName);
    }
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
    this->ComputePipelineModule = this->computePipelineManager->RegisterNewComputePipeline(*this, this->descriptorSetLayouts);
}

void ShaderModule::createShadowShaderModule(const std::string& filename_compute)
{
    this->vertShaderStageInfo = createShader(deviceModule->device, filename_compute, SHADER_TYPE::VERTEX_SHADER);
    this->shaderStages.push_back(vertShaderStageInfo);
    this->CreateDescriptorSetLayout();
    this->CreateShaderBindings();
    this->ShadowPipelineModule = this->shadowPipelineManager->RegisterNewShadowPipeline(*this, this->descriptorSetLayouts, this->graphicsPipelineData);
    this->IsShadowShader = true;
}

void ShaderModule::createShadowShaderModule(const std::string& filename_vertex, const std::string& filename_fragment)
{
    vertShaderStageInfo = createShader(deviceModule->device, filename_vertex, SHADER_TYPE::VERTEX_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    this->CreateDescriptorSetLayout();
    this->CreateShaderBindings();
    this->ShadowPipelineModule = this->shadowPipelineManager->RegisterNewShadowPipeline(*this, this->descriptorSetLayouts, this->graphicsPipelineData);
    this->IsShadowShader = true;
}

void ShaderModule::createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment)
{
    vertShaderStageInfo = createShader(deviceModule->device, filename_vertex, SHADER_TYPE::VERTEX_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    this->CreateDescriptorSetLayout();
    this->CreateShaderBindings();
    this->PipelineModule = this->graphicsPipelineManager->RegisterNewGraphicsPipeline(*this, this->descriptorSetLayouts, this->graphicsPipelineData);
}

void ShaderModule::createShaderModule(const std::string& filename_vertex, const std::string& filename_geometry, const std::string& filename_fragment)
{
    vertShaderStageInfo = createShader(deviceModule->device, filename_vertex, SHADER_TYPE::VERTEX_SHADER);
    geoShaderStageInfo = createShader(deviceModule->device, filename_geometry, SHADER_TYPE::GEOMETRY_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(geoShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    this->CreateDescriptorSetLayout();
    this->CreateShaderBindings();
    this->PipelineModule = this->graphicsPipelineManager->RegisterNewGraphicsPipeline(*this, this->descriptorSetLayouts, this->graphicsPipelineData);
}

void ShaderModule::createMeshShaderModule(const std::string& filename_task, const std::string& filename_mesh, const std::string& filename_fragment)
{
    this->graphicsPipelineData.HasVertexData = false;

    if (filename_task != "")
    {
        taskShaderStageInfo = createShader(deviceModule->device, filename_task, SHADER_TYPE::TASK_SHADER);
        shaderStages.push_back(taskShaderStageInfo);
    }

    meshShaderStageInfo = createShader(deviceModule->device, filename_mesh, SHADER_TYPE::MESH_SHADER);
    fragShaderStageInfo = createShader(deviceModule->device, filename_fragment, SHADER_TYPE::FRAGMENT_SHADER);
    shaderStages.push_back(meshShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    this->CreateDescriptorSetLayout();
    this->CreateShaderBindings();
    this->PipelineModule = this->graphicsPipelineManager->RegisterNewGraphicsPipeline(*this, this->descriptorSetLayouts, this->graphicsPipelineData);
}

void ShaderModule::CleanDescriptorSetLayout()
{
    for (uint32_t i = 0; i < this->descriptorSetLayouts.size(); i++)
    {
        vkDestroyDescriptorSetLayout(deviceModule->device, this->descriptorSetLayouts.at(i), nullptr);
        this->descriptorSetLayouts.at(i) = VK_NULL_HANDLE;
    }
}

void ShaderModule::CreateShaderBindings()
{
    this->bindingDescription = std::make_shared<VkVertexInputBindingDescription>();

    if (this->graphicsPipelineData.HasVertexData)
    {
        this->SetBindingDescription();
        this->SetAttributeDescriptions(this->attributeDescriptions);

        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = this->bindingDescription.get(); // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(this->attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = this->attributeDescriptions.data(); // Optional
    }
    else
    {
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
    }
}

void ShaderModule::cleanup()
{
    shaderStages.clear();
    if (vertex_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, vertex_shader, nullptr);
    if (fragment_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, fragment_shader, nullptr);
    if (compute_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, compute_shader, nullptr);
    if (task_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, task_shader, nullptr);
    if (mesh_shader != nullptr)
        vkDestroyShaderModule(deviceModule->device, mesh_shader, nullptr);
}

void ShaderModule::RecreatePipeline()
{
    if (this->IsShadowShader)
    {
        this->shadowPipelineManager->RecreateShadowPipeline(*this, this->descriptorSetLayouts);
    }
    else
    {
        this->graphicsPipelineManager->RecreateGraphicsPipeline(*this, this->descriptorSetLayouts);
    }
}

void ShaderModule::CleanLastResources()
{
    this->graphicsPipelineManager = nullptr;
    this->bindingDescription.reset();
    this->bindingDescription = nullptr;
    this->PipelineModule.reset();
    this->PipelineModule = nullptr;
}

VkPipelineShaderStageCreateInfo ShaderModule::createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType)
{
    std::vector<char> code = readFile(filename);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

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
            if (vkCreateShaderModule(device, &createInfo, nullptr, &geometry_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create geometry shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
            shaderStageInfo.module = geometry_shader;
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

        case SHADER_TYPE::TASK_SHADER:
            if (vkCreateShaderModule(device, &createInfo, nullptr, &task_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create task shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
            shaderStageInfo.module = task_shader;
            break;

        case SHADER_TYPE::MESH_SHADER:
            if (vkCreateShaderModule(device, &createInfo, nullptr, &mesh_shader) != VK_SUCCESS) {
                throw std::runtime_error("failed to create mesh shader module!");
            }

            shaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
            shaderStageInfo.module = mesh_shader;
            break;

        default:
            break;
    }
    shaderStageInfo.pName = "main";

    return shaderStageInfo;
}

void ShaderModule::CreateDescriptorSetLayout()
{
    this->descriptorSetLayouts.resize(this->reflectShader.bindings.size());

    int idSet = 0;
    int countSet = 0;

    while(countSet < this->reflectShader.bindings.size())
    {
        if (this->reflectShader.bindings.find(idSet) == this->reflectShader.bindings.end())
        {
            idSet++;
            continue;
        }

        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        for (auto reflectLayotBinding : this->reflectShader.bindings[idSet])
        {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding = reflectLayotBinding.second.binding;
            layoutBinding.descriptorType = reflectLayotBinding.second.type;
            layoutBinding.descriptorCount = reflectLayotBinding.second.arraySize;
            layoutBinding.stageFlags = reflectLayotBinding.second.stage;
            layoutBinding.pImmutableSamplers = nullptr;
            bindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(deviceModule->device, &layoutInfo, nullptr, &this->descriptorSetLayouts.at(countSet)) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        countSet++;
        idSet++;
    }
}

void ShaderModule::SetBindingDescription()
{
    bindingDescription->binding = 0;
    bindingDescription->stride = this->graphicsPipelineData.vertexBufferStride;
    bindingDescription->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void ShaderModule::SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
{
    size_t numAttributes = this->reflectShader.inputVariables.size();
    attributeDescriptions.resize(numAttributes);

    uint32_t offset = 0;

    for (uint16_t id = 0; id < numAttributes; id++)
    {
        attributeDescriptions[id].binding = 0;
        attributeDescriptions[id].location = this->reflectShader.inputVariables[id].location;
        attributeDescriptions[id].format = this->reflectShader.inputVariables[id].format;
        attributeDescriptions[id].offset = offset;
        offset += this->reflectShader.inputVariables[id].size;
    }
}
