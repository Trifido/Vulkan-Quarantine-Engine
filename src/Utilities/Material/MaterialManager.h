#pragma once

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <TextureManager.h>
#include <QESingleton.h>
#include <MaterialDto.h>

class QEMaterial;
class QECamera;
class LightManager;
class RenderPassModule;
class ShaderModule;

class MaterialManager : public QESingleton<MaterialManager>
{
private:
    friend class QESingleton<MaterialManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<QEMaterial>> _materials;

    LightManager*       lightManager;
    RenderPassModule*   renderPassModule;

    std::shared_ptr<ShaderModule> default_shader;
    std::shared_ptr<ShaderModule> default_primitive_shader;
    std::shared_ptr<ShaderModule> default_particles_shader;
    std::shared_ptr<ShaderModule> mesh_shader_test;
    std::shared_ptr<ShaderModule> shader_aabb_ptr;
    std::shared_ptr<ShaderModule> shader_debug_ptr;
    std::shared_ptr<ShaderModule> shader_grid_ptr;

public:
    std::shared_ptr<ShaderModule> csm_shader;
    std::shared_ptr<ShaderModule> omni_shadow_mapping_shader;

private:
    void CreateDefaultPrimitiveMaterial();
public:
    MaterialManager();
    void InitializeMaterialManager();
    std::shared_ptr<QEMaterial> GetMaterial(std::string nameMaterial);
    void AddMaterial(std::shared_ptr<QEMaterial> mat_ptr);
    void AddMaterial(QEMaterial mat);
    std::string CheckName(std::string nameMaterial);
    void CreateMaterial(std::string& nameMaterial);
    void CreateMeshShaderMaterial(std::string& nameMaterial);
    bool Exists(std::string materialName);
    void CleanPipelines();
    void CleanLastResources();
    void UpdateUniforms();
    static std::vector<MaterialDto> GetMaterialDtos(std::ifstream& file);
    static MaterialDto ReadQEMaterial(std::ifstream& file);
    void LoadMaterialDtos(std::vector<MaterialDto>& materialDtos);
    void SaveMaterials(std::ofstream& file);
};

#endif
