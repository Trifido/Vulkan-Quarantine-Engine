#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "Texture.h"
#include <GraphicsPipelineModule.h>
#include <RenderPassModule.h>

class Material : public GameComponent
{
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

    VkPipeline             pipeline;
    VkPipelineLayout       pipelineLayout;

private:
    std::shared_ptr<ShaderModule>           shader;
    std::shared_ptr<DescriptorModule>       descriptor;
    std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule;
public:
    Material();
    Material(std::shared_ptr<ShaderModule> shader_ptr, std::shared_ptr<DescriptorModule> descriptor_ptr);

    void AddTexture(std::shared_ptr<Texture> texture);

    void cleanup();
    void cleanupTextures();
    void initPipelineMaterial(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr, VkRenderPass renderPass);
    void recreatePipelineMaterial(VkRenderPass renderPass);
};

#endif // !MATERIAL_H



