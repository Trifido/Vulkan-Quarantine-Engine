#pragma once

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include <TextureManager.h>
#include <QESingleton.h>
#include <MaterialDto.h>
#include <yaml-cpp/yaml.h>
#include <unordered_set>

class QEMaterial;
class QECamera;
class RenderPassModule;
class ShaderModule;

class MaterialManager : public QESingleton<MaterialManager>
{
private:
    friend class QESingleton<MaterialManager>;

    std::unordered_map<std::string, std::shared_ptr<QEMaterial>> _materials;
    std::unordered_set<std::string> _persistentMaterialNames;

    RenderPassModule* renderPassModule;

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
    std::shared_ptr<QEMaterial> LoadMaterialFromFile(const std::filesystem::path& materialPath);
    bool ReloadShaderAsset(const std::filesystem::path& shaderAssetPath);
    bool RemoveMaterialAsset(const std::filesystem::path& materialPath);
    bool SyncRenamedMaterialAsset(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    void AddMaterial(std::shared_ptr<QEMaterial> mat_ptr);
    void AddMaterial(QEMaterial mat);

    std::string CheckName(std::string nameMaterial);
    void CreateMaterial(std::string& nameMaterial);
    void CreateMeshShaderMaterial(std::string& nameMaterial);

    bool Exists(std::string materialName);

    void MarkMaterialPersistent(const std::string& materialName);
    bool IsPersistentMaterial(const std::string& materialName) const;
    void ResetSceneState();

    void CleanPipelines();
    void CleanLastResources();
    void UpdateUniforms();

    static std::vector<MaterialDto> GetMaterialDtos(std::ifstream& file);
    static MaterialDto ReadQEMaterial(std::ifstream& file);
    void LoadMaterialDtos(std::vector<MaterialDto>& materialDtos);
    void SaveMaterials(std::ofstream& file);
    YAML::Node SerializeMaterials();
    void DeserializeMaterials(YAML::Node materials);
};

#endif
