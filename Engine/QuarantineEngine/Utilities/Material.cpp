#include "Material.h"

Material::Material()
{
    ambient = diffuse = specular = emissive = glm::vec3(0.0f);
    this->descriptor = std::make_shared<DescriptorModule>(DescriptorModule());
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr, VkRenderPass renderPass)
{
    ambient = diffuse = specular = emissive = glm::vec3(0.0f);
    this->shader = shader_ptr;
    this->renderPass = renderPass;
    this->descriptor = std::make_shared<DescriptorModule>(DescriptorModule());
    this->textures = std::make_shared < std::map<TEXTURE_TYPE, std::shared_ptr<Texture>> >();
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

    std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>::iterator it = textures->find(texture->type);
    if (it == textures->end())
    {
        textures->insert(std::pair<TEXTURE_TYPE, std::shared_ptr<Texture>>(texture->type, texture));
    }
    else
    {
        it->second = texture;
    }
}

void Material::AddPipeline(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr)
{
    this->graphicsPipelineModule = graphicsPipelineModule_ptr;
}

void Material::cleanup()
{
    this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout);

    this->descriptor->cleanupDescriptorBuffer();
    this->descriptor->cleanupDescriptorPool();
}

void Material::cleanupDescriptor()
{
    this->descriptor->cleanup();
}

void Material::recreatePipelineMaterial(VkRenderPass renderPass)
{
    this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
}

void Material::bindingMesh(std::shared_ptr<GeometryComponent> mesh)
{
    this->shader->createShaderBindings(mesh);
    this->isMeshBinding = true;
}

void Material::InitializeMaterial()
{
    if (!this->isMeshBinding)
    {
        std::cout << "Falta enlazaar el material con un gameobject.\n";
    }
    else if (this->shader == nullptr)
    {
        std::cout << "Falta shader en el material.\n";
    }
    else if (this->renderPass == nullptr)
    {
        std::cout << "Falta renderpass en el material.\n";
    }
    else if (this->graphicsPipelineModule == nullptr)
    {
        std::cout << "Falta graphicsPipelineModule en el material.\n";
    }
    else
    {
        this->descriptor->Initialize(this->textures);
        this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
    }
}

void Material::RecreateUniformsMaterial()
{
    this->descriptor->recreateUniformBuffer();
}
