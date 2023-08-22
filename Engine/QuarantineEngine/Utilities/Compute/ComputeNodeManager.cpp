#include "ComputeNodeManager.h"
#include <ShaderManager.h>

ComputeNodeManager* ComputeNodeManager::instance = nullptr;

std::string ComputeNodeManager::CheckName(std::string nameComputeNode)
{
    std::unordered_map<std::string, std::shared_ptr<ComputeNode>>::const_iterator got;

    std::string newName = nameComputeNode;
    unsigned int id = 0;

    do
    {
        got = _computeNodes.find(newName);

        if (got != _computeNodes.end())
        {
            id++;
            newName = nameComputeNode + "_" + std::to_string(id);
        }
    } while (got != _computeNodes.end());

    return newName;
}

ComputeNodeManager::ComputeNodeManager()
{
    auto shaderManager = ShaderManager::getInstance();

    this->compute_default_shader = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/Compute/default_compute.spv"));
    shaderManager->AddShader("default_compute", this->compute_default_shader);
}

ComputeNodeManager* ComputeNodeManager::getInstance()
{
    if (instance == NULL)
        instance = new ComputeNodeManager();

    return instance;
}

void ComputeNodeManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

void ComputeNodeManager::CreateComputeNode(std::string& nameComputeNode, bool hasAnimation)
{
    nameComputeNode = CheckName(nameComputeNode);
    this->AddComputeNode(nameComputeNode, std::make_shared<ComputeNode>(ComputeNode(this->compute_default_shader)));
}

void ComputeNodeManager::InitializeComputeNodeManager(std::shared_ptr<ComputePipelineModule> computePipeline)
{
    this->computePipelineModule = computePipeline;
}

void ComputeNodeManager::InitializeComputeNodes()
{
    for (auto it : _computeNodes)
    {
        it.second->InitializeComputeNode();
    }
}

void ComputeNodeManager::AddComputeNode(std::string& nameComputeNode, ComputeNode mat)
{
    std::shared_ptr<ComputeNode> mat_ptr = std::make_shared<ComputeNode>(mat);
    mat_ptr->AddComputePipeline(this->computePipelineModule);
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(const char* nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    mat_ptr->AddComputePipeline(this->computePipelineModule);
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(std::string& nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    mat_ptr->AddComputePipeline(this->computePipelineModule);
    _computeNodes[nameComputeNode] = mat_ptr;
}

std::shared_ptr<ComputeNode> ComputeNodeManager::GetComputeNode(std::string nameComputeNode)
{
    if (_computeNodes.empty())
        return nullptr;

    std::unordered_map<std::string, std::shared_ptr<ComputeNode>>::const_iterator got = _computeNodes.find(nameComputeNode);

    if (got == _computeNodes.end())
        return nullptr;

    return _computeNodes[nameComputeNode];
}

void ComputeNodeManager::RecordComputeNodes(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
    for (auto it : _computeNodes)
    {
        it.second->BindCommandBuffer(commandBuffer, currentFrame);
    }
}

void ComputeNodeManager::CleanLastResources()
{
    this->_computeNodes.clear();
    this->compute_default_shader.reset();
    this->compute_default_shader = nullptr;
    this->computePipelineModule.reset();
    this->computePipelineModule = nullptr;
}
