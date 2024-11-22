#include "Material.h"

Material::Material()
{
    this->materialData = {};
    this->layer = (unsigned int) RenderLayer::SOLID;
    this->lightManager = LightManager::getInstance();
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
        this->descriptor->ubos["materialUBO"] = this->materialData.materialUBO;
        this->descriptor->uboSizes["materialUBO"] = this->materialData.materialUniformSize;
        this->descriptor->textures = this->materialData.texture_vector;
        this->IsInitialized = true;
    }
}

void Material::cleanup()
{
    if (this->hasDescriptorBuffer)
    {
        this->descriptor->Cleanup();
    }
    this->materialData.CleanMaterialUBO();
}

void Material::bindingMesh(std::shared_ptr<GeometryComponent> mesh)
{
    this->isMeshBinding = true;
}

void Material::InitializeMaterial()
{
    if (this->hasDescriptorBuffer && this->IsInitialized)
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

void Material::SetMeshShaderPipeline(bool value)
{
    if (this->isMeshShaderEnabled != value)
    {
        this->isMeshShaderEnabled = value;
        if (this->hasDescriptorBuffer)
        {
            this->descriptor->Cleanup();
            this->descriptor.reset();
            this->descriptor = nullptr;
        }

        if (this->isMeshShaderEnabled)
        {
            if (this->hasDescriptorBuffer)
            {
                auto shaderManager = ShaderManager::getInstance();
                auto meshShader = shaderManager->GetShader("mesh_shader");
                this->descriptor = std::make_shared<DescriptorBuffer>(meshShader);
                this->materialData.InitializeUBOMaterial(meshShader);
            }
        }
        else
        {
            this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
            this->materialData.InitializeUBOMaterial(this->shader);
        }
    }
}

void Material::BindDescriptors(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->HasDescriptorBuffer())
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->shader->PipelineModule->pipelineLayout, 0, 1, descriptor->getDescriptorSet(idx), 0, nullptr);
    }

    if (this->shader->reflectShader.HasPointShadows)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->shader->PipelineModule->pipelineLayout, 1, 1, &lightManager->PointShadowDescritors->renderDescriptorSets[idx], 0, nullptr);
    }
}
