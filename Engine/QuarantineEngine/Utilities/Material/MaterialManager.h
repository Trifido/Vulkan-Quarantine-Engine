#pragma once

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <Material/Material.h>
#include <CameraEditor.h>

class MaterialManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<Material>> _materials;
    LightManager* lightManager;
    Camera* cameraEditor;
    std::shared_ptr<ShaderModule> default_shader;
    std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule;
    VkRenderPass default_renderPass;
public:
    static MaterialManager* instance;

private:
    std::string CheckName(std::string nameMaterial);

public:
    MaterialManager();
    void InitializeMaterialManager(VkRenderPass renderPass, std::shared_ptr<GraphicsPipelineModule> graphicsPipeline);
    static MaterialManager* getInstance();
    std::shared_ptr<Material> GetMaterial(std::string nameMaterial);
    void AddMaterial(std::string nameMaterial, std::shared_ptr<Material> mat_ptr);
    void AddMaterial(std::string nameMaterial, Material mat);
    void CreateMaterial(std::string nameMaterial);
    bool Exists(std::string materialName);
    void CleanDescriptors();
    void CleanPipelines();
    void RecreateMaterials(RenderPassModule* renderPassModule);
    void UpdateUniforms(uint32_t imageIndex);
    void InitializeMaterials();
};

#endif
