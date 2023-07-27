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

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    std::shared_ptr<CustomTexture> emptyTexture = nullptr;
    std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> texture_vector = nullptr;
    std::shared_ptr<MaterialUniform> uniform = nullptr;

    int numTextures;
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emissive;
    float shininess;

    static const int TOTAL_NUM_TEXTURES = 5;
    std::shared_ptr<CustomTexture> diffuseTexture = nullptr;
    std::shared_ptr<CustomTexture> normalTexture = nullptr;
    std::shared_ptr<CustomTexture> specularTexture = nullptr;
    std::shared_ptr<CustomTexture> emissiveTexture = nullptr;
    std::shared_ptr<CustomTexture> heightTexture = nullptr;

    std::shared_ptr<DescriptorModule> descriptor = nullptr;

    unsigned int layer;

    std::shared_ptr<ShaderModule> shader = nullptr;
public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr);

    void AddTexture(std::shared_ptr<CustomTexture> texture);
    void AddNullTexture(std::shared_ptr<CustomTexture> texture);

    void cleanup();
    void cleanupDescriptor();
    inline std::shared_ptr<DescriptorModule> GetDescr�ptor() { return descriptor; }
    void recreatePipelineMaterial(VkRenderPass renderPass);
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void bindingCamera(Camera* editorCamera);
    void bindingLights(LightManager* lightManager);
    void InitializeMaterial();
    void InitializeDescriptor();
    void RecreateUniformsMaterial();

private:
    std::shared_ptr<CustomTexture> findTextureByType(TEXTURE_TYPE newtype);
    void fillEmptyTextures();
    void updateUniformData();
};

#endif // !MATERIAL_H



