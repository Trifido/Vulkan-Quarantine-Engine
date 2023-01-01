#include "Material.h"

Material::Material()
{
    ambient = diffuse = specular = emissive = glm::vec3(0.0f);
    this->shininess = 32.0f;
    this->uniform = std::make_shared<MaterialUniform>();
    this->uniform->idxDiffuse = this->uniform->idxNormal = this->uniform->idxSpecular = this->uniform->idxEmissive = this->uniform->idxHeight = -1; // this->uniform->idxBump
    this->uniform->shininess = this->shininess;
    this->texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    this->texture_vector->resize(this->TOTAL_NUM_TEXTURES, nullptr);
    this->numTextures = 0;
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr, VkRenderPass renderPass)
{
    ambient = diffuse = specular = emissive = glm::vec3(0.0f);
    this->shininess = 32.0f;
    this->shader = shader_ptr;
    this->renderPass = renderPass;
    this->uniform = std::make_shared<MaterialUniform>();
    this->uniform->idxDiffuse = this->uniform->idxNormal = this->uniform->idxSpecular = this->uniform->idxEmissive = this->uniform->idxHeight = -1; //this->uniform->idxBump 
    this->uniform->shininess = this->shininess;
    this->texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    this->texture_vector->resize(this->TOTAL_NUM_TEXTURES, nullptr);
    this->numTextures = 0;
}

void Material::AddTexture(std::shared_ptr<CustomTexture> texture)
{
    bool isInserted = false;

    std::shared_ptr<CustomTexture> ptrTexture = this->findTextureByType(texture->type);
    if (ptrTexture == nullptr)
    {
        texture_vector->at(this->numTextures) = texture;
        isInserted = true;
    }
    else
    {
        ptrTexture = texture;
    }

    switch (texture->type)
    {
        case TEXTURE_TYPE::DIFFUSE_TYPE:
        default:
            diffuseTexture = texture;
            if (isInserted) this->uniform->idxDiffuse = this->numTextures++;
            break;
        case TEXTURE_TYPE::NORMAL_TYPE:
            normalTexture = texture;
            if (isInserted) this->uniform->idxNormal = this->numTextures++;
            break;
        case TEXTURE_TYPE::SPECULAR_TYPE:
            specularTexture = texture;
            if (isInserted) this->uniform->idxSpecular = this->numTextures++;
            break;
        case TEXTURE_TYPE::HEIGHT_TYPE:
            heightTexture = texture;
            if (isInserted) this->uniform->idxHeight = this->numTextures++;
            break;
        case TEXTURE_TYPE::EMISSIVE_TYPE:
            emissiveTexture = texture;
            if (isInserted) this->uniform->idxEmissive = this->numTextures++;
            break;
        //case TEXTURE_TYPE::BUMP_TYPE:
        //    bumpTexture = texture;
        //    if (isInserted) this->uniform->idxBump = this->numTextures++;
        //    break;
    }
}

void Material::AddNullTexture(std::shared_ptr<CustomTexture> texture)
{
    this->emptyTexture = texture;
}

void Material::AddPipeline(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule_ptr)
{
    this->graphicsPipelineModule = graphicsPipelineModule_ptr;
}

void Material::cleanup()
{
    if (this->isMeshBinding)
    {
        this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout);
        this->descriptor->cleanupDescriptorBuffer();
        this->descriptor->cleanupDescriptorPool();
    }
}

void Material::cleanupDescriptor()
{
    this->descriptor->cleanup();
}

void Material::recreatePipelineMaterial(VkRenderPass renderPass)
{
    if (this->isMeshBinding)
    {
        this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
    }
}

void Material::bindingCamera(Camera* editorCamera)
{
    this->descriptor->cameraUniform = editorCamera->cameraUniform;
}

void Material::bindingLights(LightManager* lightManager)
{
    this->descriptor->lightUniform = lightManager->lightManagerUniform;
}

void Material::bindingMesh(std::shared_ptr<GeometryComponent> mesh)
{
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
        this->fillEmptyTextures();
        //std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> texture_vector_aux = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
        //texture_vector_aux->push_back(texture_vector->at(0));
        this->descriptor->Initialize(texture_vector, uniform);
        this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
    }
}

void Material::InitializeDescriptor()
{
    this->descriptor = std::make_shared<DescriptorModule>();//new DescriptorModule()
}

std::shared_ptr<CustomTexture> Material::findTextureByType(TEXTURE_TYPE newtype)
{
    for (size_t id = 0; id < texture_vector->size(); id++)
    {
        if (texture_vector->at(id) != nullptr)
        {
            if (texture_vector->at(id)->type == newtype)
                return texture_vector->at(id);
        }
        else
        {
            return nullptr;
        }
    }
    return nullptr;
}

void Material::RecreateUniformsMaterial()
{
    if (this->isMeshBinding)
    {
        this->descriptor->recreateUniformBuffer();
    }
}

void Material::fillEmptyTextures()
{
    for (size_t i = 0; i < this->texture_vector->size(); i++)
    {
        if (this->texture_vector->at(i) == nullptr)
        {
            this->texture_vector->at(i) = emptyTexture;
        }
    }
}

void Material::updateUniformData()
{
    this->uniform->shininess = this->shininess;
}
