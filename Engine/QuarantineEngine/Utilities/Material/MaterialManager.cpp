#include "MaterialManager.h"
#include "ShaderManager.h"
#include <GraphicsPipelineModule.h>
#include <filesystem>
#include <Vertex.h>

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
    this->renderPassModule = RenderPassModule::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_default_vertex_shader_path = absPath + "/Default/default_vert.spv";
    const std::string absolute_default_frag_shader_path = absPath + "/Default/default_frag.spv";
    const std::string absolute_csm_vertex_shader_path = absPath + "/Shadow/csm_vert.spv";
    const std::string absolute_csm_frag_shader_path = absPath + "/Shadow/csm_frag.spv";
    const std::string absolute_omni_shadow_vertex_shader_path = absPath + "/Shadow/omni_shadow_vert.spv";
    const std::string absolute_omni_shadow_frag_shader_path = absPath + "/Shadow/omni_shadow_frag.spv";
    const std::string absolute_particles_vert_shader_path = absPath + "/Particles/particles_vert.spv";
    const std::string absolute_particles_frag_shader_path = absPath + "/Particles/particles_frag.spv";
    const std::string absolute_mesh_task_shader_path = absPath + "/mesh/mesh_task.spv";
    const std::string absolute_mesh_mesh_shader_path = absPath + "/mesh/mesh_mesh.spv";
    const std::string absolute_mesh_frag_shader_path = absPath + "/mesh/mesh_frag.spv";

    this->lightManager = LightManager::getInstance();
    this->cameraEditor = CameraEditor::getInstance();

    auto shaderManager = ShaderManager::getInstance();
    this->default_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_default_vertex_shader_path, absolute_default_frag_shader_path)
    );
    shaderManager->AddShader("default", this->default_shader);

    this->default_primitive_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_default_vertex_shader_path, absolute_default_frag_shader_path)
    );
    shaderManager->AddShader("default_primitive", this->default_primitive_shader);

    GraphicsPipelineData pipelineParticleShader = {};
    pipelineParticleShader.HasVertexData = false;
    this->default_particles_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_particles_vert_shader_path, absolute_particles_frag_shader_path, pipelineParticleShader)
    );
    shaderManager->AddShader("default_particles", this->default_particles_shader);

    GraphicsPipelineData pipelineMeshShader = {};
    pipelineMeshShader.HasVertexData = false;
    pipelineMeshShader.IsMeshShader = true;
    this->mesh_shader_test = std::make_shared<ShaderModule>(
        ShaderModule(absolute_mesh_task_shader_path, absolute_mesh_mesh_shader_path, absolute_mesh_frag_shader_path, pipelineMeshShader)
    );
    shaderManager->AddShader("mesh_shader", this->mesh_shader_test);

    GraphicsPipelineData pipelineShadowShader = {};

    pipelineShadowShader.shadowMode = ShadowMappingMode::DIRECTIONAL_SHADOW;
    pipelineShadowShader.renderPass = this->renderPassModule->dirShadowMappingRenderPass;
    this->csm_shader = std::make_shared<ShaderModule>(ShaderModule(absolute_csm_vertex_shader_path, absolute_csm_frag_shader_path, pipelineShadowShader));
    shaderManager->AddShader("csm_shader", this->csm_shader);

    pipelineShadowShader.shadowMode = ShadowMappingMode::OMNI_SHADOW;
    pipelineShadowShader.renderPass = this->renderPassModule->omniShadowMappingRenderPass;
    this->omni_shadow_mapping_shader = std::make_shared<ShaderModule>(ShaderModule(absolute_omni_shadow_vertex_shader_path, absolute_omni_shadow_frag_shader_path, pipelineShadowShader));
    shaderManager->AddShader("omni_shadow_mapping_shader", this->omni_shadow_mapping_shader);
}

void MaterialManager::InitializeMaterialManager()
{
    this->CreateDefaultPrimitiveMaterial();

    if (this->default_particles_shader != nullptr)
    {
        this->AddMaterial("defaultParticlesMat", std::make_shared<Material>(Material(this->default_particles_shader)));
        this->_materials["defaultParticlesMat"]->layer = (int)RenderLayer::PARTICLES;
    }
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
    nameMaterial = CheckName(nameMaterial);
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::AddMaterial(const char* nameMaterial, std::shared_ptr<Material> mat_ptr)
{
    std::string name = nameMaterial;
    name = CheckName(name);
    _materials[name] = mat_ptr;
}

void MaterialManager::AddMaterial(std::string& nameMaterial, Material mat)
{
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(mat);
    nameMaterial = CheckName(nameMaterial);
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::AddMaterial(const char* nameMaterial, Material mat)
{
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(mat);
    std::string name = nameMaterial;
    name = CheckName(name);
    _materials[name] = mat_ptr;
}


void MaterialManager::CreateDefaultPrimitiveMaterial()
{
    if (this->default_primitive_shader != nullptr)
    {
        this->AddMaterial("defaultPrimitiveMat", std::make_shared<Material>(Material(this->default_primitive_shader)));
    }

    if (this->mesh_shader_test != nullptr)
    {
        this->AddMaterial("defaultMeshPrimitiveMat", std::make_shared<Material>(Material(this->mesh_shader_test)));
        _materials["defaultMeshPrimitiveMat"]->SetMeshShaderPipeline(true);
    }
}

void MaterialManager::CreateMaterial(std::string& nameMaterial, bool hasAnimation)
{
    nameMaterial = CheckName(nameMaterial);
    return this->AddMaterial(nameMaterial, std::make_shared<Material>(Material(this->default_shader)));
}

void MaterialManager::CreateMeshShaderMaterial(std::string& nameMaterial)
{
    nameMaterial = CheckName(nameMaterial);
    this->AddMaterial(nameMaterial, std::make_shared<Material>(Material(this->mesh_shader_test)));
    //_materials[nameMaterial]->SetMeshShaderPipeline(true);
}

bool MaterialManager::Exists(std::string materialName)
{
    std::unordered_map<std::string, std::shared_ptr<Material>>::const_iterator got = _materials.find(materialName);

    if (got == _materials.end())
        return false;

    return true;
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

    for (auto mat : this->_materials)
    {
        mat.second->CleanLastResources();
        mat.second.reset();
    }

    this->_materials.clear();
    this->cameraEditor = nullptr;
    this->lightManager = nullptr;
    this->default_shader.reset();
    this->default_primitive_shader.reset();
    this->default_shader = nullptr;
    this->default_primitive_shader = nullptr;
    this->mesh_shader_test.reset();
    this->mesh_shader_test = nullptr;
    this->csm_shader.reset();
    this->csm_shader = nullptr;
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
