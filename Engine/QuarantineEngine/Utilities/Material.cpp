#include "Material.h"

Material::Material()
{
    ambient = diffuse = specular = emissive = glm::vec3(0.0f);
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr, std::shared_ptr<DescriptorModule> descriptor_ptr)
{
    this->shader = shader_ptr;
    this->descriptor = descriptor_ptr;
}

void Material::AddTexture(std::shared_ptr<Texture> texture)
{
    switch (texture->type)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:
    default:
        diffuseTexture = texture;
        break;

    case TEXTURE_TYPE::NORMAL_TYPE:
        normalTexture = texture;
        break;
    case TEXTURE_TYPE::SPECULAR_TYPE:
        specularTexture = texture;
        break;
    case TEXTURE_TYPE::HEIGHT_TYPE:
        heightTexture = texture;
        break;
    case TEXTURE_TYPE::EMISSIVE_TYPE:
        emissiveTexture = texture;
        break;
    case TEXTURE_TYPE::BUMP_TYPE:
        bumpTexture = texture;
        break;
    }
}

void Material::cleanup()
{
    //this->albedo_ptr->cleanup();
    this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout, this->shader);
}

void Material::cleanupTextures()
{
    this->diffuseTexture->cleanup();
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
