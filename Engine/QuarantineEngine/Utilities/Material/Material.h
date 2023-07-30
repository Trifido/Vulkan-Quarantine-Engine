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
    //std::shared_ptr<MaterialUniform> uniform = nullptr;
    DescriptorBuffer descriptor;

public:
    MaterialData materialData;
    unsigned int layer;
    std::shared_ptr<ShaderModule> shader = nullptr;

public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr);

    void cleanup();
    void cleanupDescriptor();
    void recreatePipelineMaterial(VkRenderPass renderPass);
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void bindingCamera(Camera* editorCamera);
    void bindingLights(LightManager* lightManager);
    void InitializeMaterial();
    void InitializeDescriptor();
    void RecreateUniformsMaterial();

private:
    void updateUniformData();
};

#endif // !MATERIAL_H



