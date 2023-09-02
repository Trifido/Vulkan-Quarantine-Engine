#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include <RenderLayer.h>
#include <ShaderModule.h>
#include <MaterialData.h>
#include <DescriptorBuffer.h>

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    bool hasDescriptorBuffer = false;

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
    void cleanupDescriptor();
    std::shared_ptr<Material> CreateMaterialInstance();
    void recreatePipelineMaterial(VkRenderPass renderPass);
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void bindingCamera(Camera* editorCamera);
    void bindingLights(LightManager* lightManager);
    void InitializeMaterial();
    void InitializeDescriptor();
    void InitializeMaterialDataUBO();
    void RecreateUniformsMaterial();
    void UpdateUniformData();
    bool HasDescriptorBuffer() { return this->hasDescriptorBuffer; }
};

#endif // !MATERIAL_H



