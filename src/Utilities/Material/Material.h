#pragma once

#ifndef QE_MATERIAL_H
#define QE_MATERIAL_H

#include <glm/glm.hpp>
#include <RenderLayer.h>
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

public:
    std::string Name;
    MaterialData materialData;
    unsigned int layer;
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
    bool HasDescriptorBuffer() { return this->hasDescriptorBuffer; }
    void SetMeshShaderPipeline(bool value);
    void BindDescriptors(VkCommandBuffer& commandBuffer, uint32_t idx);
    void RenameMaterial(std::string newName);
    std::string SaveMaterialFile();
};

#endif // !MATERIAL_H



