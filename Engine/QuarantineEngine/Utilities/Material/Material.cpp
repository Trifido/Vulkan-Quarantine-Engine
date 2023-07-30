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
    this->descriptor = DescriptorBuffer(this->shader);
}

void Material::cleanup()
{
    if (this->isMeshBinding)
    {
        //this->graphicsPipelineModule->cleanup(this->pipeline, this->pipelineLayout);
        //this->descriptor->cleanupDescriptorBuffer();
        //this->descriptor->cleanupDescriptorPool();
    }
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

void Material::updateUniformData()
{
    //this->uniform->shininess = this->shininess;
}
