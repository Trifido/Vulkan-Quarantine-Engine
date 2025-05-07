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
    const std::string absolute_debugBB_vertex_shader_path = absPath + "/Debug/debugAABB_vert.spv";
    const std::string absolute_debugBB_frag_shader_path = absPath + "/Debug/debugAABB_frag.spv";
    const std::string absolute_debug_vertex_shader_path = absPath + "/Debug/debug_vert.spv";
    const std::string absolute_debug_frag_shader_path = absPath + "/Debug/debug_frag.spv";
    const std::string absolute_grid_vertex_shader_path = absPath + "/Grid/grid_vert.spv";
    const std::string absolute_grid_frag_shader_path = absPath + "/Grid/grid_frag.spv";

    this->lightManager = LightManager::getInstance();
    this->cameraEditor = CameraEditor::getInstance();

    auto shaderManager = ShaderManager::getInstance();
    this->default_shader = std::make_shared<ShaderModule>(
        ShaderModule("default", absolute_default_vertex_shader_path, absolute_default_frag_shader_path)
    );
    shaderManager->AddShader(this->default_shader);

    this->default_primitive_shader = std::make_shared<ShaderModule>(
        ShaderModule("default_primitive", absolute_default_vertex_shader_path, absolute_default_frag_shader_path)
    );
    shaderManager->AddShader(this->default_primitive_shader);

    GraphicsPipelineData pipelineParticleShader = {};
    pipelineParticleShader.HasVertexData = false;
    this->default_particles_shader = std::make_shared<ShaderModule>(
        ShaderModule("default_particles", absolute_particles_vert_shader_path, absolute_particles_frag_shader_path, pipelineParticleShader)
    );
    shaderManager->AddShader(this->default_particles_shader);

    //GraphicsPipelineData pipelineMeshShader = {};
    //pipelineMeshShader.HasVertexData = false;
    //pipelineMeshShader.IsMeshShader = true;
    //this->mesh_shader_test = std::make_shared<ShaderModule>(
    //    ShaderModule("mesh_shader", absolute_mesh_task_shader_path, absolute_mesh_mesh_shader_path, absolute_mesh_frag_shader_path, pipelineMeshShader)
    //);
    //shaderManager->AddShader(this->mesh_shader_test);

    GraphicsPipelineData pipelineShadowShader = {};

    pipelineShadowShader.shadowMode = ShadowMappingMode::DIRECTIONAL_SHADOW;
    pipelineShadowShader.renderPass = this->renderPassModule->dirShadowMappingRenderPass;
    this->csm_shader = std::make_shared<ShaderModule>(ShaderModule("csm_shader", absolute_csm_vertex_shader_path, absolute_csm_frag_shader_path, pipelineShadowShader));
    shaderManager->AddShader(this->csm_shader);

    pipelineShadowShader.shadowMode = ShadowMappingMode::OMNI_SHADOW;
    pipelineShadowShader.renderPass = this->renderPassModule->omniShadowMappingRenderPass;
    this->omni_shadow_mapping_shader = std::make_shared<ShaderModule>(ShaderModule("omni_shadow_mapping_shader", absolute_omni_shadow_vertex_shader_path, absolute_omni_shadow_frag_shader_path, pipelineShadowShader));
    shaderManager->AddShader(this->omni_shadow_mapping_shader);

    GraphicsPipelineData gpData = {};
    gpData.HasVertexData = true;
    gpData.polygonMode = VK_POLYGON_MODE_LINE;
    gpData.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    gpData.vertexBufferStride = sizeof(glm::vec4);
    gpData.lineWidth = 2.0f;

    this->shader_aabb_ptr = std::make_shared<ShaderModule>(ShaderModule("shader_aabb_debug", absolute_debugBB_vertex_shader_path, absolute_debugBB_frag_shader_path, gpData));
    shaderManager->AddShader(shader_aabb_ptr);

    {
        gpData.HasVertexData = true;
        gpData.polygonMode = VK_POLYGON_MODE_LINE;
        gpData.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        gpData.vertexBufferStride = sizeof(DebugVertex);
        gpData.lineWidth = 1.0f;

        this->shader_debug_ptr = std::make_shared<ShaderModule>(ShaderModule("shader_debug_lines", absolute_debug_vertex_shader_path, absolute_debug_frag_shader_path, gpData));
        shaderManager->AddShader(shader_debug_ptr);
    }

    gpData = {};
    gpData.HasVertexData = false;

    this->shader_grid_ptr = std::make_shared<ShaderModule>(ShaderModule("shader_grid", absolute_grid_vertex_shader_path, absolute_grid_frag_shader_path, gpData));
    shaderManager->AddShader(shader_grid_ptr);
}

void MaterialManager::InitializeMaterialManager()
{
    this->CreateDefaultPrimitiveMaterial();

    if (!this->Exists("defaultParticlesMat"))
    {
        if (this->default_particles_shader != nullptr)
        {
            this->AddMaterial(std::make_shared<Material>(Material("defaultParticlesMat", this->default_particles_shader)));
            this->_materials["defaultParticlesMat"]->layer = (int)RenderLayer::PARTICLES;
        }
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

void MaterialManager::AddMaterial(std::shared_ptr<Material> mat_ptr)
{
    std::string nameMaterial = CheckName(mat_ptr->Name);
    mat_ptr->RenameMaterial(nameMaterial);
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::AddMaterial(Material mat)
{
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(mat);
    std::string nameMaterial = CheckName(mat.Name);
    mat_ptr->Name = nameMaterial;
    _materials[nameMaterial] = mat_ptr;
}

void MaterialManager::CreateDefaultPrimitiveMaterial()
{
    if (!this->Exists("defaultPrimitiveMat"))
    {
        if (this->default_primitive_shader != nullptr)
        {
            this->AddMaterial(std::make_shared<Material>(Material("defaultPrimitiveMat", this->default_primitive_shader)));
        }
    }

    if (!this->Exists("defaultMeshPrimitiveMat"))
    {
        if (this->mesh_shader_test != nullptr)
        {
            this->AddMaterial(std::make_shared<Material>(Material("defaultMeshPrimitiveMat", this->mesh_shader_test)));
            _materials["defaultMeshPrimitiveMat"]->SetMeshShaderPipeline(true);
        }
    }
}

void MaterialManager::CreateMaterial(std::string& nameMaterial)
{
    nameMaterial = CheckName(nameMaterial);
    this->AddMaterial(std::make_shared<Material>(Material(nameMaterial, this->default_shader)));
}

void MaterialManager::CreateMeshShaderMaterial(std::string& nameMaterial)
{
    nameMaterial = CheckName(nameMaterial);
    this->AddMaterial(std::make_shared<Material>(Material(nameMaterial, this->mesh_shader_test)));
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

std::vector<MaterialDto> MaterialManager::GetMaterialDtos(std::ifstream& file)
{
    // Read the materials
    int numMaterials;
    file.read(reinterpret_cast<char*>(&numMaterials), sizeof(int));

    int materialPathLength;

    std::vector<std::string> materialPaths(numMaterials);

    for (int i = 0; i < numMaterials; i++)
    {
        file.read(reinterpret_cast<char*>(&materialPathLength), sizeof(materialPathLength));
        materialPaths[i].resize(materialPathLength);
        file.read(&materialPaths[i][0], materialPathLength);
    }

    try
    {
        std::vector<MaterialDto> materialDtos;
    
        for (int i = 0; i < numMaterials; i++)
        {
            std::ifstream matfile(materialPaths[i], std::ios::binary);
            if (!matfile.is_open())
            {
                std::cerr << "Error al abrir el material " << materialPaths[i] << std::endl;
                continue;
            }

            MaterialDto materialDto = ReadQEMaterial(matfile);

            matfile.close();

            materialDtos.push_back(materialDto);
        }

        return materialDtos;
    }
    catch (const std::bad_alloc& e)
    {
        std::cerr << "Error al asignar memoria para los materiales: " << e.what() << std::endl;
        return {};
    }
}

MaterialDto MaterialManager::ReadQEMaterial(std::ifstream& matfile)
{
    int materialPathLength;
    int shaderPathLength;
    int nameLength;
    int diffuseLength;
    int normalLength;
    int specularLength;
    int emissiveLength;
    int heightLength;

    MaterialDto materialDto {};

    matfile.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
    materialDto.Name.resize(nameLength);
    matfile.read(&materialDto.Name[0], nameLength);

    matfile.read(reinterpret_cast<char*>(&materialPathLength), sizeof(materialPathLength));
    materialDto.FilePath.resize(materialPathLength);
    matfile.read(&materialDto.FilePath[0], materialPathLength);

    matfile.read(reinterpret_cast<char*>(&shaderPathLength), sizeof(shaderPathLength));
    materialDto.ShaderPath.resize(shaderPathLength);
    matfile.read(&materialDto.ShaderPath[0], shaderPathLength);

    matfile.read(reinterpret_cast<char*>(&materialDto.layer), sizeof(int));

    matfile.read(reinterpret_cast<char*>(&materialDto.Opacity), sizeof(float));
    matfile.read(reinterpret_cast<char*>(&materialDto.BumpScaling), sizeof(float));
    matfile.read(reinterpret_cast<char*>(&materialDto.Shininess), sizeof(float));
    matfile.read(reinterpret_cast<char*>(&materialDto.Reflectivity), sizeof(float));
    matfile.read(reinterpret_cast<char*>(&materialDto.Shininess_Strength), sizeof(float));
    matfile.read(reinterpret_cast<char*>(&materialDto.Refractivity), sizeof(float));

    matfile.read(reinterpret_cast<char*>(&materialDto.Diffuse[0]), sizeof(glm::vec4));
    matfile.read(reinterpret_cast<char*>(&materialDto.Ambient), sizeof(glm::vec4));
    matfile.read(reinterpret_cast<char*>(&materialDto.Specular), sizeof(glm::vec4));
    matfile.read(reinterpret_cast<char*>(&materialDto.Emissive), sizeof(glm::vec4));
    matfile.read(reinterpret_cast<char*>(&materialDto.Transparent), sizeof(glm::vec4));
    matfile.read(reinterpret_cast<char*>(&materialDto.Reflective), sizeof(glm::vec4));

    matfile.read(reinterpret_cast<char*>(&diffuseLength), sizeof(diffuseLength));
    if (diffuseLength > 0)
    {
        materialDto.diffuseTexturePath.resize(diffuseLength);
        matfile.read(&materialDto.diffuseTexturePath[0], diffuseLength);
    }

    matfile.read(reinterpret_cast<char*>(&normalLength), sizeof(normalLength));
    if (normalLength > 0)
    {
        materialDto.normalTexturePath.resize(normalLength);
        matfile.read(&materialDto.normalTexturePath[0], normalLength);
    }

    matfile.read(reinterpret_cast<char*>(&specularLength), sizeof(specularLength));
    if (specularLength > 0)
    {
        materialDto.specularTexturePath.resize(specularLength);
        matfile.read(&materialDto.specularTexturePath[0], specularLength);
    }

    matfile.read(reinterpret_cast<char*>(&emissiveLength), sizeof(emissiveLength));
    if (emissiveLength > 0)
    {
        materialDto.emissiveTexturePath.resize(emissiveLength);
        matfile.read(&materialDto.emissiveTexturePath[0], emissiveLength);
    }

    matfile.read(reinterpret_cast<char*>(&heightLength), sizeof(heightLength));
    if (heightLength > 0)
    {
        materialDto.heightTexturePath.resize(heightLength);
        matfile.read(&materialDto.heightTexturePath[0], heightLength);
    }

    return materialDto;
}

void MaterialManager::LoadMaterialDtos(std::vector<MaterialDto>& materialDtos)
{
    auto shaderManager = ShaderManager::getInstance();
    for (auto& it : materialDtos)
    {
        auto shader = shaderManager->GetShader(it.ShaderPath);

        if (!this->Exists(it.Name))
        {
            this->AddMaterial(std::make_shared<Material>(Material(shader, it)));
        }
    }
}

void MaterialManager::SaveMaterials(std::ofstream& file)
{
    int materialCount = static_cast<int>(_materials.size());
    file.write(reinterpret_cast<const char*>(&materialCount), sizeof(int));

    for (auto& it : _materials)
    {
        std::string materialPath = it.second->SaveMaterialFile();

        int materialPathLength = static_cast<int>(materialPath.length());

        if (materialPathLength > 0)
        {
            file.write(reinterpret_cast<const char*>(&materialPathLength), sizeof(int));
            file.write(materialPath.c_str(), materialPathLength);
        }
    }
}
