#pragma once

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <Material/Material.h>
#include <CameraEditor.h>
#include <Material/TextureManager.h>
#include <GraphicsPipelineModule.h>
#include <RenderPassModule.h>

class MaterialManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<Material>> _materials;

    Camera*             cameraEditor;
    LightManager*       lightManager;
    RenderPassModule*   renderPassModule;

    std::shared_ptr<ShaderModule> default_shader;
    std::shared_ptr<ShaderModule> default_primitive_shader;
    std::shared_ptr<ShaderModule> default_particles_shader;
    std::shared_ptr<ShaderModule> mesh_shader_test;

public:
    static MaterialManager* instance;
    std::shared_ptr<ShaderModule> csm_shader;
    std::shared_ptr<ShaderModule> omni_shadow_mapping_shader;

private:
    std::string CheckName(std::string nameMaterial);
    void CreateDefaultPrimitiveMaterial();
public:
    MaterialManager();
    void InitializeMaterialManager();
    static MaterialManager* getInstance();
    static void ResetInstance();
    std::shared_ptr<Material> GetMaterial(std::string nameMaterial);
    void AddMaterial(const char* nameMaterial, std::shared_ptr<Material> mat_ptr);
    void AddMaterial(std::string& nameMaterial, std::shared_ptr<Material> mat_ptr);
    void AddMaterial(std::string& nameMaterial, Material mat);
    void AddMaterial(const char* nameMaterial, Material mat);
    void CreateMaterial(std::string& nameMaterial, bool hasAnimation);
    void CreateMeshShaderMaterial(std::string& nameMaterial);
    bool Exists(std::string materialName);
    void CleanPipelines();
    void CleanLastResources();
    void UpdateUniforms();
    void InitializeMaterials();
};

#endif
