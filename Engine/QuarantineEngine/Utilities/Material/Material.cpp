#include "Material.h"

Material::Material()
{
    this->materialData = {};

    //this->uniform = std::make_shared<MaterialUniform>();
    //this->uniform->idxDiffuse = this->uniform->idxNormal = this->uniform->idxSpecular = this->uniform->idxEmissive = this->uniform->idxHeight = -1;
    //this->uniform->shininess = this->shininess;

    this->layer = (unsigned int) RenderLayer::SOLID;
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr) : Material()
{
    this->shader = shader_ptr;
    this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
}

void Material::CleanLastResources()
{
    this->materialData.CleanLastResources();
    this->descriptor->CleanLastResources();
    this->descriptor.reset();
    this->descriptor = nullptr;
    this->shader->CleanLastResources();
    this->shader.reset();
    this->shader = nullptr;
}

void Material::InitializeMaterialDataUBO()
{
    this->materialData.InitializeUBOMaterial(this->shader);
    this->descriptor->materialUBO = this->materialData.materialUBO;
    this->descriptor->materialUniformSize = this->materialData.materialUniformSize;
    this->descriptor->textures = this->materialData.texture_vector;
}

void Material::cleanup()
{
    //if (this->isMeshBinding)
    //{
        //this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout);
        //this->descriptor->cleanupDescriptorBuffer();
        //this->descriptor->cleanupDescriptorPool();
    //}
    this->descriptor->CleanDescriptorSetPool();
    this->materialData.CleanMaterialUBO();
}

void Material::cleanupDescriptor()
{
    //this->descriptor->cleanup();
}

void Material::recreatePipelineMaterial(VkRenderPass renderPass)
{
    if (this->isMeshBinding)
    {
        //this->graphicsPipelineModule->CreateGraphicsPipeline(this->pipeline, this->pipelineLayout, this->shader, this->descriptor, renderPass);
    }
}

void Material::bindingCamera(Camera* editorCamera)
{
    //this->descriptor->cameraUniform = editorCamera->cameraUniform;
}

void Material::bindingLights(LightManager* lightManager)
{
    //this->descriptor->lightUniform = lightManager->lightManagerUniform;
}

void Material::bindingMesh(std::shared_ptr<GeometryComponent> mesh)
{
    this->isMeshBinding = true;
}

void Material::InitializeMaterial()
{
    if (!this->isMeshBinding)
    {
        std::cout << "Falta enlazar el material con un gameobject.\n";
    }
    else if (this->shader == nullptr)
    {
        std::cout << "Falta shader en el material.\n";
    }
    else
    {
        this->descriptor->InitializeDescriptorSets(this->shader);
        //this->fillEmptyTextures();
        //this->descriptor->Initialize(texture_vector, uniform);
    }
}

void Material::InitializeDescriptor()
{
    //this->descriptor = std::make_shared<DescriptorModule>();
}

void Material::RecreateUniformsMaterial()
{
    if (this->isMeshBinding)
    {
        //this->descriptor->recreateUniformBuffer();
    }
}

void Material::UpdateUniformData()
{
    if (this->isMeshBinding && this->shader != nullptr)
    {
        this->materialData.UpdateUBOMaterial();
    }
    //this->uniform->shininess = this->shininess;
}
