#include "ComputeNodeManager.h"
#include <ShaderManager.h>
#include <filesystem>

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

void ComputeNodeManager::InitializeComputeResources()
{
    auto shaderManager = ShaderManager::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders/").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_default_compute_shader_path = absPath + "Compute/default_compute.spv";
    const std::string absolute_animation_compute_shader_path = absPath + "Animation/computeSkinning.spv";

    shaderManager->AddShader("default_compute_particles", std::make_shared<ShaderModule>(ShaderModule(absolute_default_compute_shader_path)));
    shaderManager->AddShader("default_skinning", std::make_shared<ShaderModule>(ShaderModule(absolute_animation_compute_shader_path)));
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
    nameComputeNode = CheckName(nameComputeNode);
    std::shared_ptr<ComputeNode> mat_ptr = std::make_shared<ComputeNode>(mat);
    _computeNodes[nameComputeNode] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(const char* nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    std::string name = nameComputeNode;
    name = CheckName(name);
    _computeNodes[name] = mat_ptr;
}

void ComputeNodeManager::AddComputeNode(std::string& nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr)
{
    nameComputeNode = CheckName(nameComputeNode);
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

void ComputeNodeManager::Cleanup()
{
    for each (auto node in _computeNodes)
    {
        node.second->cleanup();
    }
}

void ComputeNodeManager::CleanLastResources()
{
    this->_computeNodes.clear();
}
