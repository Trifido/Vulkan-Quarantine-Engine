#pragma once

#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <ShaderModule.h>
#include <QESingleton.h>

class ShaderManager : public QESingleton<ShaderManager>
{
private:
    friend class QESingleton<ShaderManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<ShaderModule>> _shaders;

private:
    std::string CheckName(std::string nameMaterial);

public:
    std::shared_ptr<ShaderModule> GetShader(std::string nameMaterial);
    void AddShader(std::shared_ptr<ShaderModule> shader_ptr);
    void AddShader(ShaderModule shader);
    bool Exists(std::string shaderName);
    void Clean();
    void CleanDescriptorSetLayouts();
    void CleanLastResources();
    void RecreateShaderGraphicsPipelines();
};

#endif
