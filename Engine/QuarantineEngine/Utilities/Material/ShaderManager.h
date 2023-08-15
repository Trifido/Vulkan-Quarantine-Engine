#pragma once

#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <ShaderModule.h>

class ShaderManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>> _shaders;

public:
    static ShaderManager* instance;

private:
    std::string CheckName(std::string nameMaterial);

public:
    static ShaderManager* getInstance();
    std::shared_ptr<ShaderModule> GetShader(std::string nameMaterial);
    void AddShader(std::string shaderName, std::shared_ptr<ShaderModule> shader_ptr);
    void AddShader(std::string shaderName, ShaderModule shader);
    bool Exists(std::string shaderName);
    void Clean();
    void CleanDescriptorSetLayouts();
};

#endif
