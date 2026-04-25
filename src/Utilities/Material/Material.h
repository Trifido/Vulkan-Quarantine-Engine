#pragma once

#ifndef QE_MATERIAL_H
#define QE_MATERIAL_H

#include <glm/glm.hpp>
#include <RenderQueue.h>
#include <ShaderManager.h>
#include <ShaderModule.h>
#include <MaterialData.h>
#include <DescriptorBuffer.h>
#include <LightManager.h>
#include <MaterialDto.h>

class QEMaterial : public Numbered
{
private:
    bool IsInitialized = false;
    bool hasDescriptorBuffer = false;
    bool isMeshShaderEnabled = false;
    LightManager* lightManager;
    std::string materialFilePath;
    std::string shaderAssetPath;

    static std::string ToMaterialRelativePath(
        const std::string& assetPath,
        const std::filesystem::path& materialFilePath);

public:
    std::string Name;
    MaterialData materialData;
    unsigned int renderQueue = static_cast<unsigned int>(RenderQueue::Geometry);
    std::shared_ptr<ShaderModule> shader;
    std::shared_ptr<DescriptorBuffer> descriptor;

public:
    QEMaterial(std::string name, std::string filepath = "");
    QEMaterial(std::string name, std::shared_ptr<ShaderModule> shader_ptr, std::string filepath = "");
    QEMaterial(std::shared_ptr<ShaderModule> shader_ptr, const MaterialDto& materialDto);
    void CleanLastResources();

    void cleanup();
    std::shared_ptr<QEMaterial> CreateMaterialInstance();
    void InitializeMaterialData();
    void UpdateUniformData();
    void RefreshDescriptorBindings();
    bool HasDescriptorBuffer() { return this->hasDescriptorBuffer; }
    void SetMeshShaderPipeline(bool value);
    void BindDescriptors(VkCommandBuffer& commandBuffer, uint32_t idx);
    void RenameMaterial(std::string newName);
    std::string SaveMaterialFile();
    MaterialDto ToDto() const;
    bool ApplyShader(const std::shared_ptr<ShaderModule>& shaderPtr, const std::string& assetPath = "");
    void SetShaderAssetPath(const std::string& path) { shaderAssetPath = path; }
    const std::string& GetShaderAssetPath() const { return shaderAssetPath; }
    const std::string& GetMaterialFilePath() const { return materialFilePath; }
    void SetMaterialFilePath(const std::string& path) { materialFilePath = path; }
};

#endif // !MATERIAL_H



