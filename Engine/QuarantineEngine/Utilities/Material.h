#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "Texture.h"
#include <GraphicsPipelineModule.h>
#include <RenderPassModule.h>
#include <map>

class Material : public GameComponent
{
private:
    bool isMeshBinding = false;
    std::shared_ptr<std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textures;
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emissive;

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
    void AddPipeline(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr);

    void cleanup();
    void cleanupDescriptor();
    void recreatePipelineMaterial(VkRenderPass renderPass);
    void bindingMesh(std::shared_ptr<GeometryComponent> mesh);
    void InitializeMaterial();
    void RecreateUniformsMaterial();
};

#endif // !MATERIAL_H



