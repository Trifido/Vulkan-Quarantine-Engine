#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "QEGameComponent.h"
#include <RenderLayer.h>
#include <ShaderManager.h>
#include <ShaderModule.h>
#include <MaterialData.h>
#include <DescriptorBuffer.h>
#include <LightManager.h>
#include <MaterialDto.h>

class Material : public QEGameComponent
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
    Material(std::string name, std::string filepath = "");
    Material(std::string name, std::shared_ptr<ShaderModule> shader_ptr, std::string filepath = "");
    Material(std::shared_ptr<ShaderModule> shader_ptr, const MaterialDto& materialDto);
    void CleanLastResources();

    void cleanup();
    std::shared_ptr<Material> CreateMaterialInstance();
    void InitializeMaterial();
    void InitializeMaterialDataUBO();
    void UpdateUniformData();
    bool HasDescriptorBuffer() { return this->hasDescriptorBuffer; }
    void SetMeshShaderPipeline(bool value);
    void BindDescriptors(VkCommandBuffer& commandBuffer, uint32_t idx);
    void RenameMaterial(std::string newName);
    std::string SaveMaterialFile();

    void QEStart() override;
    void QEUpdate() override;
    void QERelease() override;
};

#endif // !MATERIAL_H



