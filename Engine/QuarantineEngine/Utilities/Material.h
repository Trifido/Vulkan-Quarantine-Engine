#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "CustomTexture.h"
#include <GraphicsPipelineModule.h>
#include <RenderPassModule.h>
#include <map>
#include <UBO.h>
#include "Camera.h"
#include <LightManager.h>

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    std::shared_ptr<CustomTexture> emptyTexture;
    std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> texture_vector;
    std::shared_ptr<MaterialUniform> uniform;

    const int TOTAL_NUM_TEXTURES = 6;
    int numTextures;
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emissive;
    float shininess;

    std::shared_ptr<CustomTexture> diffuseTexture;
    std::shared_ptr<CustomTexture> normalTexture;
    std::shared_ptr<CustomTexture> specularTexture;
    std::shared_ptr<CustomTexture> emissiveTexture;
    std::shared_ptr<CustomTexture> heightTexture;
    std::shared_ptr<CustomTexture> bumpTexture;

    VkRenderPass           renderPass;
    VkPipeline             pipeline;
    VkPipelineLayout       pipelineLayout;

    std::shared_ptr<ShaderModule>           shader;
    std::shared_ptr<DescriptorModule>       descriptor;
    std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule;
public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr, VkRenderPass renderPass);

    void AddTexture(std::shared_ptr<CustomTexture> texture);
    void AddNullTexture(std::shared_ptr<CustomTexture> texture);
    void AddPipeline(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr);

    void cleanup();
    void cleanupDescriptor();
    void recreatePipelineMaterial(VkRenderPass renderPass);
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void bindingCamera(std::shared_ptr<Camera> editorCamera);
    void bindingLights(std::shared_ptr<LightManager> lightManager);
    void InitializeMaterial();
    void RecreateUniformsMaterial();

private:
    std::shared_ptr<CustomTexture> findTextureByType(TEXTURE_TYPE newtype);
    void fillEmptyTextures();
    void updateUniformData();
};

#endif // !MATERIAL_H



