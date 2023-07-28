#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "CustomTexture.h"
#include <RenderPassModule.h>
#include <map>
#include <UBO.h>
#include "Camera.h"
#include <LightManager.h>
#include <RenderLayer.h>
#include <GraphicsPipelineManager.h>
#include <ShaderModule.h>
#include <DescriptorModule.h>
#include <MaterialData.h>

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    std::shared_ptr<MaterialUniform> uniform = nullptr;

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



