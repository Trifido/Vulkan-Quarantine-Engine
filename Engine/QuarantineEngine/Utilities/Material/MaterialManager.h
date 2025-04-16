#pragma once

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <Material/Material.h>
#include <CameraEditor.h>
#include <Material/TextureManager.h>
#include <GraphicsPipelineModule.h>
#include <RenderPassModule.h>
#include <QESingleton.h>
#include <MaterialDto.h>

class MaterialManager : public QESingleton<MaterialManager>
{
private:
    friend class QESingleton<MaterialManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<Material>> _materials;

    Camera*             cameraEditor;
    LightManager*       lightManager;
    RenderPassModule*   renderPassModule;

    std::shared_ptr<ShaderModule> default_shader;
    std::shared_ptr<ShaderModule> default_primitive_shader;
    std::shared_ptr<ShaderModule> default_particles_shader;
    std::shared_ptr<ShaderModule> mesh_shader_test;
    std::shared_ptr<ShaderModule> shader_aabb_ptr;
    std::shared_ptr<ShaderModule> shader_grid_ptr;

public:
    std::shared_ptr<ShaderModule> csm_shader;
    std::shared_ptr<ShaderModule> omni_shadow_mapping_shader;

private:
    void CreateDefaultPrimitiveMaterial();
public:
    MaterialManager();
    void InitializeMaterialManager();
    std::shared_ptr<Material> GetMaterial(std::string nameMaterial);
    void AddMaterial(std::shared_ptr<Material> mat_ptr);
    void AddMaterial(Material mat);
    std::string CheckName(std::string nameMaterial);
    void CreateMaterial(std::string& nameMaterial);
    void CreateMeshShaderMaterial(std::string& nameMaterial);
    bool Exists(std::string materialName);
    void CleanPipelines();
    void CleanLastResources();
    void UpdateUniforms();
    void InitializeMaterials();
    static std::vector<MaterialDto> GetMaterialDtos(std::ifstream& file);
    void LoadMaterialDtos(std::vector<MaterialDto>& materialDtos);
    void SaveMaterials(std::ofstream& file);
};

#endif
