#include "ShaderManager.h"

std::string ShaderManager::CheckName(std::string nameMaterial)
{
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>>::const_iterator got;

    std::string newName = nameMaterial;
    unsigned int id = 0;

    do
    {
        got = _shaders.find(newName);

        if (got != _shaders.end())
        {
            id++;
            newName = nameMaterial + "_" + std::to_string(id);
        }
    } while (got != _shaders.end());

    return newName;
}

std::shared_ptr<ShaderModule> ShaderManager::GetShader(std::string shaderName)
{
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>>::const_iterator got = _shaders.find(shaderName);

    if (got == _shaders.end())
        return nullptr;

    return _shaders[shaderName];
}

void ShaderManager::AddShader(std::shared_ptr<ShaderModule> shader_ptr)
{
    std::string name = CheckName(shader_ptr->shaderNameID);
    _shaders[name] = shader_ptr;
}

void ShaderManager::AddShader(ShaderModule shader)
{
    std::string name = CheckName(shader.shaderNameID);
    _shaders[name] = std::make_shared<ShaderModule>(shader);
}

bool ShaderManager::Exists(std::string shaderName)
{
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>>::const_iterator got = _shaders.find(shaderName);

    if (got == _shaders.end())
        return false;

    return true;
}

void ShaderManager::Clean()
{
    for (auto& it : _shaders)
    {
        it.second->cleanup();
    }
}

void ShaderManager::CleanDescriptorSetLayouts()
{
    for (auto shader : this->_shaders)
    {
        shader.second->CleanDescriptorSetLayout();
    }
}

void ShaderManager::CleanLastResources()
{
    this->_shaders.clear();
}

void ShaderManager::RecreateShaderGraphicsPipelines()
{
    for (auto shader : this->_shaders)
    {
        if (shader.second->shaderStages.size() > 1)
        {
            shader.second->RecreatePipeline();
        }
    }
}
