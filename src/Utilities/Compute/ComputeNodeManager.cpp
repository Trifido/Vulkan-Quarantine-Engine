#include "ComputeNodeManager.h"
#include <ShaderManager.h>
#include <filesystem>

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

void ComputeNodeManager::InitializeComputeResources()
{
    auto shaderManager = ShaderManager::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders/").generic_string();

    const std::string absolute_emit_compute_shader_path = absPath + "Particles/emitParticles.spv";
    const std::string absolute_update_compute_shader_path = absPath + "Particles/updateParticles.spv";
    const std::string absolute_animation_compute_shader_path = absPath + "Animation/computeSkinning.spv";
    const std::string transmittance_lut_compute_shader_path = absPath + "Atmosphere/transmittance_LUT.spv";
    const std::string multi_scattering_lut_compute_shader_path = absPath + "Atmosphere/multi_scattering_LUT.spv";
    const std::string sky_view_lut_compute_shader_path = absPath + "Atmosphere/sky_view_LUT.spv";

    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("emit_compute_particles", absolute_emit_compute_shader_path)));
    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("update_compute_particles", absolute_update_compute_shader_path)));
    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("default_skinning", absolute_animation_compute_shader_path)));
    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("transmittance_lut", transmittance_lut_compute_shader_path)));
    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("multi_scattering_lut", multi_scattering_lut_compute_shader_path)));
    shaderManager->AddShader(std::make_shared<ShaderModule>(ShaderModule("sky_view_lut", sky_view_lut_compute_shader_path)));
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
    for (auto node : _computeNodes)
    {
        node.second->cleanup();
    }
}

void ComputeNodeManager::CleanLastResources()
{
    this->_computeNodes.clear();
}
