#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include <RenderLayer.h>
#include <ShaderManager.h>
#include <ShaderModule.h>
#include <MaterialData.h>
#include <DescriptorBuffer.h>

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    bool IsInitialized = false;
    bool hasDescriptorBuffer = false;
    bool isMeshShaderEnabled = false;

public:
    MaterialData materialData;
    unsigned int layer;
    std::shared_ptr<ShaderModule> shader = nullptr;
    std::shared_ptr<DescriptorBuffer> descriptor;

public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr);
    void CleanLastResources();

    void cleanup();
    std::shared_ptr<Material> CreateMaterialInstance();
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void InitializeMaterial();
    void InitializeMaterialDataUBO();
    void UpdateUniformData();
    bool HasDescriptorBuffer() { return this->hasDescriptorBuffer; }
    void SetMeshShaderPipeline(bool value);
};

#endif // !MATERIAL_H



