#include "MaterialManager.h"
#include "ShaderManager.h"
#include <GraphicsPipelineModule.h>

MaterialManager* MaterialManager::instance = nullptr;

std::string MaterialManager::CheckName(std::string nameMaterial)
{
    std::unordered_map<std::string, std::shared_ptr<Material>>::const_iterator got;

    std::string newName = nameMaterial;
    unsigned int id = 0;

    do
    {
        got = _materials.find(newName);

        if (got != _materials.end())
        {
            id++;
            newName = nameMaterial + "_" + std::to_string(id);
        }
    } while (got != _materials.end());

    return newName;
}

MaterialManager::MaterialManager()
{
    this->lightManager = LightManager::getInstance();
    this->cameraEditor = CameraEditor::getInstance();

    auto shaderManager = ShaderManager::getInstance();
    this->default_shader = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv"));
    shaderManager->AddShader("default", this->default_shader);

    this->default_primitive_shader = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/Primitive/primitiveDefault_vert.spv", "../../resources/shaders/Primitive/primitiveDefault_frag.spv"));
    shaderManager->AddShader("default_primitive", this->default_primitive_shader);

    this->default_animation_shader = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/Animation/exampleAnimated_vert.spv", "../../resources/shaders/Animation/exampleAnimated_frag.spv"));
    shaderManager->AddShader("default_animation", this->default_animation_shader);
}

void MaterialManager::InitializeMaterialManager()
{
    this->CreateDefaultPrimitiveMaterial();
}

MaterialManager* MaterialManager::getInstance()
{
    if (instance == NULL)
        instance = new MaterialManager();
    else
        std::cout << "Getting existing instance of Material Manager" << std::endl;

    return instance;
}

std::shared_ptr<Material> MaterialManager::GetMaterial(std::string nameMaterial)
{
    if (_materials.empty())
        return nullptr;

    std::unordered_map<std::string, std::shared_ptr<Material>>::const_iterator got = _materials.find(nameMaterial);

    if (got == _materials.end())
        return nullptr;

    return _materials[nameMaterial];
}

void MaterialManager::AddMaterial(std::string& nameMaterial, std::shared_ptr<Material> mat_ptr)
{
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::AddMaterial(const char* nameMaterial, std::shared_ptr<Material> mat_ptr)
{
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::AddMaterial(std::string& nameMaterial, Material mat)
{
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(mat);
    _materials[nameMaterial] = mat_ptr;
}


void MaterialManager::CreateDefaultPrimitiveMaterial()
{
    this->AddMaterial(std::string("defaultPrimitiveMat"), std::make_shared<Material>(Material(this->default_primitive_shader)));
}

void MaterialManager::CreateMaterial(std::string& nameMaterial, bool hasAnimation)                                                                          // --------------------- En desarrollo ------------------------
{
    nameMaterial = CheckName(nameMaterial);

    if (!hasAnimation)
    {
        return this->AddMaterial(nameMaterial, std::make_shared<Material>(Material(this->default_shader)));
    }
    else
    {
        return this->AddMaterial(nameMaterial, std::make_shared<Material>(Material(this->default_animation_shader)));
    }
}

bool MaterialManager::Exists(std::string materialName)
{
    std::unordered_map<std::string, std::shared_ptr<Material>>::const_iterator got = _materials.find(materialName);

    if (got == _materials.end())
        return false;

    return true;
}

void MaterialManager::CleanDescriptors()
{
    for (auto& it : _materials)
    {
        it.second->cleanupDescriptor();
    }
}

void MaterialManager::CleanPipelines()
{
    for (auto& it : _materials)
    {
        it.second->cleanup();
    }
}

void MaterialManager::RecreateMaterials(RenderPassModule* renderPassModule)
{
    for (auto& it : _materials)
    {
        it.second->recreatePipelineMaterial(renderPassModule->renderPass);
        it.second->RecreateUniformsMaterial();
    }
}

void MaterialManager::UpdateUniforms()
{
    for (auto& it : _materials)
    {
        it.second->materialData.UpdateUBOMaterial();
        //it.second->GetDescrìptor()->updateUniforms(imageIndex);
    }
}

void MaterialManager::InitializeMaterials()
{
    //for (auto it : _materials)
    //{
    //    it.second->bindingCamera(this->cameraEditor);
    //    it.second->bindingLights(this->lightManager);
    //    it.second->InitializeMaterial();
    //}
}
