#include "MaterialManager.h"
#include "ShaderManager.h"

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
    this->textureManager = TextureManager::getInstance();

    auto shaderManager = ShaderManager::getInstance();
    //std::shared_ptr<ShaderModule> shader_ptr = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv"));
    //this->shaderManager->AddShader("shader", shader_ptr);
    this->default_shader = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/vert.spv", "../../resources/shaders/frag.spv"));
    this->default_shader->createShaderBindings();
    shaderManager->AddShader("default", this->default_shader);
}

void MaterialManager::InitializeMaterialManager(VkRenderPass renderPass, std::shared_ptr<GraphicsPipelineModule> graphicsPipeline)
{
    this->default_renderPass = renderPass;
    this->graphicsPipelineModule = graphicsPipeline;
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

void MaterialManager::AddMaterial(std::string nameMaterial, std::shared_ptr<Material> mat_ptr)
{
    std::string name = CheckName(nameMaterial);
    mat_ptr->AddPipeline(this->graphicsPipelineModule);
    _materials[name] = mat_ptr;
    _materials[name]->AddNullTexture(this->textureManager->GetTexture("NULL_TEXTURE"));
}

void MaterialManager::AddMaterial(std::string nameMaterial, Material mat)
{
    std::string name = CheckName(nameMaterial);
    std::shared_ptr<Material> mat_ptr = std::make_shared<Material>(mat);
    mat_ptr->AddPipeline(this->graphicsPipelineModule);
    _materials[name] = mat_ptr;
    _materials[name]->AddNullTexture(this->textureManager->GetTexture("NULL_TEXTURE"));
}

void MaterialManager::CreateMaterial(std::string nameMaterial)                                                                          // --------------------- En desarrollo ------------------------
{
    this->AddMaterial(nameMaterial, std::make_shared<Material>(Material(this->default_shader, this->default_renderPass)));
    
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

void MaterialManager::UpdateUniforms(uint32_t imageIndex)
{
    for (auto& it : _materials)
    {
        it.second->descriptor->updateUniforms(imageIndex);
    }
}

void MaterialManager::InitializeMaterials()
{
    for (auto& it : _materials)
    {
        it.second->bindingCamera(this->cameraEditor);
        it.second->bindingLights(this->lightManager);
        it.second->InitializeMaterial();
    }
}