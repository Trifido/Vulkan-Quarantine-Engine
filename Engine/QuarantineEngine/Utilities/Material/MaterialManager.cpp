#include "MaterialManager.h"
#include "ShaderManager.h"
#include <GraphicsPipelineModule.h>
#include <filesystem>

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
    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_default_vertex_shader_path = absPath + "/vert.spv";
    const std::string absolute_default_frag_shader_path = absPath + "/frag.spv";
    const std::string absolute_primitive_vertex_shader_path = absPath + "/Primitive/primitiveDefault_vert.spv";
    const std::string absolute_primitive_frag_shader_path = absPath + "/Primitive/primitiveDefault_frag.spv";
    const std::string absolute_animation_vertex_shader_path = absPath + "/Animation/exampleAnimated_vert.spv";
    const std::string absolute_animation_frag_shader_path = absPath + "/Animation/exampleAnimated_frag.spv";

    std::cout << absolute_default_vertex_shader_path << std::endl;
    std::cout << absolute_default_frag_shader_path << std::endl;

    this->lightManager = LightManager::getInstance();
    this->cameraEditor = CameraEditor::getInstance();

    auto shaderManager = ShaderManager::getInstance();
    this->default_shader = std::make_shared<ShaderModule>(ShaderModule(absolute_default_vertex_shader_path, absolute_default_frag_shader_path));
    shaderManager->AddShader("default", this->default_shader);

    this->default_primitive_shader = std::make_shared<ShaderModule>(ShaderModule(absolute_primitive_vertex_shader_path, absolute_primitive_frag_shader_path));
    shaderManager->AddShader("default_primitive", this->default_primitive_shader);

    this->default_animation_shader = std::make_shared<ShaderModule>(ShaderModule(absolute_animation_vertex_shader_path, absolute_animation_frag_shader_path));
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

    return instance;
}

void MaterialManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
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
    if (this->default_primitive_shader != nullptr)
    {
        this->AddMaterial(std::string("defaultPrimitiveMat"), std::make_shared<Material>(Material(this->default_primitive_shader)));
    }
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
    // Clean Camera & Lights UBO's
    this->cameraEditor->CleanCameraUBO();
    this->lightManager->CleanLightUBO();
}

void MaterialManager::CleanLastResources()
{
    for each (auto mat in this->_materials)
    {
        mat.second->CleanLastResources();
        mat.second.reset();
    }
    this->_materials.clear();
    this->cameraEditor = nullptr;
    this->lightManager = nullptr;
    this->default_shader.reset();
    this->default_primitive_shader.reset();
    this->default_animation_shader.reset();
    this->default_shader = nullptr;
    this->default_primitive_shader = nullptr;
    this->default_animation_shader = nullptr;
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
        it.second->UpdateUniformData();
    }
}

void MaterialManager::InitializeMaterials()
{
    for (auto it : _materials)
    {
        it.second->InitializeMaterial();
    }
}
