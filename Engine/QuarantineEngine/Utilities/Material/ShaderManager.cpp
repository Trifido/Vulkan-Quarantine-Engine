#include "ShaderManager.h"

ShaderManager* ShaderManager::instance = nullptr;

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

ShaderManager* ShaderManager::getInstance()
{
    if (instance == NULL)
        instance = new ShaderManager();
    else
        std::cout << "Getting existing instance of Shader Manager" << std::endl;

    return instance;
}

std::shared_ptr<ShaderModule> ShaderManager::GetShader(std::string shaderName)
{
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>>::const_iterator got = _shaders.find(shaderName);

    if (got == _shaders.end())
        return nullptr;

    return _shaders[shaderName];
}

void ShaderManager::AddShader(std::string shaderName, std::shared_ptr<ShaderModule> shader_ptr)
{
    std::string name = CheckName(shaderName);
    _shaders[name] = shader_ptr;
}

void ShaderManager::AddShader(std::string shaderName, ShaderModule shader)
{
    std::string name = CheckName(shaderName);
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
    for each (auto shader in this->_shaders)
    {
        shader.second->CleanDescriptorSetLayout();
    }
}

void ShaderManager::RecreateShaderGraphicsPipelines()
{
    for each (auto shader in this->_shaders)
    {
        shader.second->RecreatePipeline();
    }
}
