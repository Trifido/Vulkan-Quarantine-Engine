#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "Texture.h"
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
    std::shared_ptr<Texture> emptyTexture;
    std::shared_ptr <std::vector<std::shared_ptr<Texture>>> texture_vector;
    std::shared_ptr<MaterialUniform> uniform;

    const int TOTAL_NUM_TEXTURES = 6;
    int numTextures;
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emissive;
    float shininess;

    std::shared_ptr<Texture> diffuseTexture;
    std::shared_ptr<Texture> normalTexture;
    std::shared_ptr<Texture> specularTexture;
    std::shared_ptr<Texture> emissiveTexture;
    std::shared_ptr<Texture> heightTexture;
    std::shared_ptr<Texture> bumpTexture;

    VkRenderPass           renderPass;
    VkPipeline             pipeline;
    VkPipelineLayout       pipelineLayout;

    std::shared_ptr<ShaderModule>           shader;
    std::shared_ptr<DescriptorModule>       descriptor;
    std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule;
public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr, VkRenderPass renderPass);

    void AddTexture(std::shared_ptr<Texture> texture);
    void AddNullTexture(std::shared_ptr<Texture> texture);
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
    std::shared_ptr<Texture> findTextureByType(TEXTURE_TYPE newtype);
    void fillEmptyTextures();
    void updateUniformData();
};

#endif // !MATERIAL_H



