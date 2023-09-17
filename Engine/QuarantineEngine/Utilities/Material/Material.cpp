#include "Material.h"

Material::Material()
{
    this->materialData = {};
    this->layer = (unsigned int) RenderLayer::SOLID;
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr) : Material()
{
    this->shader = shader_ptr;
    this->hasDescriptorBuffer = shader_ptr->reflectShader.bindings.size() > 0;

    if (this->hasDescriptorBuffer)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }
}

std::shared_ptr<Material> Material::CreateMaterialInstance()
{
    std::shared_ptr<Material> mat_instance = std::make_shared<Material>(this->shader);
    mat_instance->layer = this->layer;
    return mat_instance;
}

void Material::CleanLastResources()
{
    this->materialData.CleanLastResources();

    if (this->hasDescriptorBuffer)
    {
        this->descriptor->CleanLastResources();
        this->descriptor.reset();
        this->descriptor = nullptr;
    }
    this->shader->CleanLastResources();
    this->shader.reset();
    this->shader = nullptr;
    this->hasDescriptorBuffer = false;
}

void Material::InitializeMaterialDataUBO()
{
    if (this->hasDescriptorBuffer)
    {
        this->materialData.InitializeUBOMaterial(this->shader);
        this->descriptor->materialUBO = this->materialData.materialUBO;
        this->descriptor->materialUniformSize = this->materialData.materialUniformSize;
        this->descriptor->textures = this->materialData.texture_vector;
    }
}

void Material::cleanup()
{
    if (this->hasDescriptorBuffer)
    {
        this->descriptor->CleanDescriptorSetPool();
    }
    this->materialData.CleanMaterialUBO();
}

void Material::bindingMesh(std::shared_ptr<GeometryComponent> mesh)
{
    this->isMeshBinding = true;
}

void Material::InitializeMaterial()
{
    if(this->hasDescriptorBuffer)
    {        
        this->descriptor->InitializeDescriptorSets(this->shader);
    }
}


void Material::UpdateUniformData()
{
    if (this->isMeshBinding && this->shader != nullptr && this->hasDescriptorBuffer)
    {
        this->materialData.UpdateUBOMaterial();
    }
}
