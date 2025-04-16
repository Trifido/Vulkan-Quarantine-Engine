#include "Material.h"
#include <QEProjectManager.h>

Material::Material(std::string name, std::string filepath)
{
    this->Name = name;

    if (filepath == "")
    {
        fs::path filenamePath = QEProjectManager::GetMaterialFolderPath();
        filenamePath /= name + ".qemat";
        this->materialFilePath = filenamePath.string();
    }
    else
    {
        this->materialFilePath = filepath;
    }

    this->materialData = {};
    this->layer = (unsigned int) RenderLayer::SOLID;
    this->lightManager = LightManager::getInstance();
}

Material::Material(std::string name, std::shared_ptr<ShaderModule> shader_ptr, std::string filepath) : Material(name, filepath)
{
    this->shader = shader_ptr;
    this->hasDescriptorBuffer = shader_ptr->reflectShader.bindings.size() > 0;

    if (this->hasDescriptorBuffer)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }
}

Material::Material(std::shared_ptr<ShaderModule> shader_ptr, const MaterialDto& materialDto) : Material(materialDto.Name, shader_ptr, materialDto.FilePath)
{
    this->layer = materialDto.layer;

    this->materialData.Opacity = materialDto.Opacity;
    this->materialData.BumpScaling = materialDto.BumpScaling;
    this->materialData.Shininess = materialDto.Shininess;
    this->materialData.Reflectivity = materialDto.Reflectivity;
    this->materialData.Shininess_Strength = materialDto.Shininess_Strength;
    this->materialData.Refractivity = materialDto.Refractivity;

    this->materialData.Diffuse = materialDto.Diffuse;
    this->materialData.Ambient = materialDto.Ambient;
    this->materialData.Specular = materialDto.Specular;
    this->materialData.Emissive = materialDto.Emissive;
    this->materialData.Transparent = materialDto.Transparent;
    this->materialData.Reflective = materialDto.Reflective;

    std::vector<std::pair<std::string, TEXTURE_TYPE>> texturePaths =
    {
        std::pair(materialDto.diffuseTexturePath, TEXTURE_TYPE::DIFFUSE_TYPE),
        std::pair(materialDto.normalTexturePath, TEXTURE_TYPE::NORMAL_TYPE),
        std::pair(materialDto.specularTexturePath, TEXTURE_TYPE::SPECULAR_TYPE),
        std::pair(materialDto.emissiveTexturePath, TEXTURE_TYPE::EMISSIVE_TYPE),
        std::pair(materialDto.heightTexturePath, TEXTURE_TYPE::HEIGHT_TYPE)
    };

    for (const auto& texturePath : texturePaths)
    {
        if (texturePath.first != "NULL_TEXTURE")
        {
            this->materialData.AddTexture(texturePath.first, std::make_shared<CustomTexture>(texturePath.first, texturePath.second));
        }
        else
        {
            this->materialData.AddTexture("NULL_TEXTURE", std::make_shared<CustomTexture>("", TEXTURE_TYPE::NULL_TYPE));
        }
    }
}

std::shared_ptr<Material> Material::CreateMaterialInstance()
{
    std::shared_ptr<Material> mat_instance = std::make_shared<Material>(this->Name, this->shader);
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
    if (this->hasDescriptorBuffer && !this->IsInitialized)
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

    if (this->shader->reflectShader.HasDirectionalShadows)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->shader->PipelineModule->pipelineLayout, 2, 1, &lightManager->CSMDescritors->renderDescriptorSets[idx], 0, nullptr);
    }
}

std::string Material::SaveMaterialFile()
{
    std::ofstream file(materialFilePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error al abrir " << materialFilePath << " para escritura.\n";
        return "";
    }

    if (file.is_open())
    {
        // Name
        int materialNameLength = Name.length();
        file.write(reinterpret_cast<const char*>(&materialNameLength), sizeof(int));
        file.write(reinterpret_cast<const char*>(Name.c_str()), materialNameLength);

        // Material file path
        int materialPathLength = materialFilePath.length();
        file.write(reinterpret_cast<const char*>(&materialPathLength), sizeof(int));
        file.write(reinterpret_cast<const char*>(materialFilePath.c_str()), materialPathLength);

        // Shader
        std::string shaderName = shader->shaderNameID;
        int shaderNameLength = shaderName.length();
        file.write(reinterpret_cast<const char*>(&shaderNameLength), sizeof(int));
        file.write(reinterpret_cast<const char*>(shaderName.c_str()), shaderNameLength);

        // Render layer
        int renderLayer = 1;
        file.write(reinterpret_cast<const char*>(&renderLayer), sizeof(int));

        // Material data
        file.write(reinterpret_cast<const char*>(&materialData.Opacity), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.BumpScaling), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.Shininess), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.Reflectivity), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.Shininess_Strength), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.Refractivity), sizeof(float));

        file.write(reinterpret_cast<const char*>(&materialData.Diffuse), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Ambient), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Specular), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Emissive), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Transparent), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Reflective), sizeof(glm::vec4));

        // Texture paths
        int zeroLength = 0;

        for (int i = 0; i < materialData.texture_vector->size(); i++)
        {
            auto texture = materialData.Textures[(TEXTURE_TYPE)i];
            if (texture != nullptr && texture->type != TEXTURE_TYPE::NULL_TYPE)
            {
                texture->SaveTexturePath(file);
            }
            else
            {
                file.write(reinterpret_cast<const char*>(&zeroLength), sizeof(int));
            }
        }

        file.close();
    }
    return this->materialFilePath;
}
