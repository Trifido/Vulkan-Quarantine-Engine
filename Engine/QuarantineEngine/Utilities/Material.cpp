#include "Material.h"

Material::Material()
{
    ambient = diffuse = specular = emission = glm::vec3(0.0f);
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr, std::shared_ptr<DescriptorModule> descriptor_ptr)
{
    this->shader = shader_ptr;
    this->descriptor = descriptor_ptr;
}

void Material::addAlbedo(std::shared_ptr<Texture> albedo)
{
    this->albedo_ptr = albedo;
}

void Material::cleanup()
{
    //this->albedo_ptr->cleanup();
    this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout, this->shader);
}

void Material::cleanupTextures()
{
    this->albedo_ptr->cleanup();
}

void Material::initPipelineMaterial(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr, VkRenderPass renderPass)
{
    this->graphicsPipelineModule = graphicsPipelineModule_ptr;
    this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
}

void Material::recreatePipelineMaterial(VkRenderPass renderPass)
{
    this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
}
