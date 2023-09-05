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
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(const char* nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(std::string& nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::UpdateComputeNodes()
{
    for each (auto node in _computeNodes)
    {
        node.second->UpdateComputeDescriptor();
    }
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
        it.second->DispatchCommandBuffer(commandBuffer, currentFrame);
    }
}

void ComputeNodeManager::CleanLastResources()
{
    this->_computeNodes.clear();
}
