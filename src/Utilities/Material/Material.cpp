#include "Material.h"
#include <QEProjectManager.h>

QEMaterial::QEMaterial(std::string name, std::string filepath)
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

QEMaterial::QEMaterial(std::string name, std::shared_ptr<ShaderModule> shader_ptr, std::string filepath) : QEMaterial(name, filepath)
{
    this->shader = shader_ptr;
    this->hasDescriptorBuffer = shader_ptr->reflectShader.bindings.size() > 0;

    if (this->hasDescriptorBuffer)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }
}

QEMaterial::QEMaterial(std::shared_ptr<ShaderModule> shader_ptr, const MaterialDto& materialDto) : QEMaterial(materialDto.Name, shader_ptr, materialDto.FilePath)
{
    this->layer = materialDto.layer;

    this->materialData.Opacity = materialDto.Opacity;
    this->materialData.BumpScaling = materialDto.BumpScaling;
    this->materialData.Shininess = materialDto.Shininess;
    this->materialData.Reflectivity = materialDto.Reflectivity;
    this->materialData.Shininess_Strength = materialDto.Shininess_Strength;
    this->materialData.Refractivity = materialDto.Refractivity;
    this->materialData.Metallic = materialDto.Metallic;
    this->materialData.Roughness = materialDto.Roughness;
    this->materialData.AO = materialDto.AO;

    this->materialData.Diffuse = materialDto.Diffuse;
    this->materialData.Ambient = materialDto.Ambient;
    this->materialData.Specular = materialDto.Specular;
    this->materialData.Emissive = materialDto.Emissive;
    this->materialData.Transparent = materialDto.Transparent;
    this->materialData.Reflective = materialDto.Reflective;

    std::vector<std::pair<std::string, TEXTURE_TYPE>> texturePaths =
    {
        { materialDto.diffuseTexturePath,   TEXTURE_TYPE::DIFFUSE_TYPE },
        { materialDto.normalTexturePath,    TEXTURE_TYPE::NORMAL_TYPE },
        { materialDto.metallicTexturePath,  TEXTURE_TYPE::METALNESS_TYPE },
        { materialDto.roughnessTexturePath, TEXTURE_TYPE::ROUGHNESS_TYPE },
        { materialDto.aoTexturePath,        TEXTURE_TYPE::AO_TYPE },
        { materialDto.emissiveTexturePath,  TEXTURE_TYPE::EMISSIVE_TYPE },
        { materialDto.heightTexturePath,    TEXTURE_TYPE::HEIGHT_TYPE },
        { materialDto.specularTexturePath,  TEXTURE_TYPE::SPECULAR_TYPE }
    };

    for (const auto& tp : texturePaths)
    {
        const std::string& path = tp.first;
        TEXTURE_TYPE type = tp.second;

        if (!path.empty() && path != "NULL_TEXTURE")
        {
            this->materialData.AddTexture(path, std::make_shared<CustomTexture>(path, type));
        }
        else
        {
            this->materialData.AddTexture("NULL_TEXTURE", std::make_shared<CustomTexture>("", TEXTURE_TYPE::NULL_TYPE));
        }
    }
}

std::shared_ptr<QEMaterial> QEMaterial::CreateMaterialInstance()
{
    std::string instanceName = "QEMatInst_" + this->Name;
    std::shared_ptr<QEMaterial> mat_instance = std::make_shared<QEMaterial>(instanceName, this->shader);
    mat_instance->layer = this->layer;
    return mat_instance;
}

void QEMaterial::CleanLastResources()
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

void QEMaterial::InitializeMaterialData()
{
    if (this->hasDescriptorBuffer && !this->IsInitialized)
    {
        this->materialData.InitializeUBOMaterial(this->shader);
        this->descriptor->ubos["materialUBO"] = this->materialData.materialUBO;
        this->descriptor->uboSizes["materialUBO"] = this->materialData.materialUniformSize;
        this->descriptor->textures = this->materialData.texture_vector;

        this->descriptor->InitializeDescriptorSets(this->shader);

        this->IsInitialized = true;
    }
}

void QEMaterial::cleanup()
{
    if (this->hasDescriptorBuffer)
    {
        this->descriptor->Cleanup();
    }
    this->materialData.CleanMaterialUBO();
}

void QEMaterial::UpdateUniformData()
{
    if (this->shader != nullptr && this->hasDescriptorBuffer)
    {
        this->materialData.UpdateUBOMaterial();
    }
}

void QEMaterial::SetMeshShaderPipeline(bool value)
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

void QEMaterial::BindDescriptors(VkCommandBuffer& commandBuffer, uint32_t idx)
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

void QEMaterial::RenameMaterial(std::string newName)
{
    if (this->Name != newName)
    {
        fs::path newFilePath = fs::path(this->materialFilePath).parent_path() / (newName + ".qemat");
        this->materialFilePath = newFilePath.string();
        this->Name = newName;
    }
}

std::string QEMaterial::SaveMaterialFile()
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
        int materialNameLength = static_cast<int>(Name.length());
        file.write(reinterpret_cast<const char*>(&materialNameLength), sizeof(int));
        file.write(reinterpret_cast<const char*>(Name.c_str()), materialNameLength);

        // Material file path
        int materialPathLength = static_cast<int>(materialFilePath.length());
        file.write(reinterpret_cast<const char*>(&materialPathLength), sizeof(int));
        file.write(reinterpret_cast<const char*>(materialFilePath.c_str()), materialPathLength);

        // Shader
        std::string shaderName = shader->shaderNameID;
        int shaderNameLength = static_cast<int>(shaderName.length());
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
        file.write(reinterpret_cast<const char*>(&materialData.Metallic), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.Roughness), sizeof(float));
        file.write(reinterpret_cast<const char*>(&materialData.AO), sizeof(float));

        file.write(reinterpret_cast<const char*>(&materialData.Diffuse), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Ambient), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Specular), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Emissive), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Transparent), sizeof(glm::vec4));
        file.write(reinterpret_cast<const char*>(&materialData.Reflective), sizeof(glm::vec4));

        // Texture paths
        int zeroLength = 0;

        const int slotCount = (int)materialData.texture_vector->size();
        for (int slot = 0; slot < slotCount; ++slot)
        {
            auto& texture = materialData.texture_vector->at((size_t)slot);

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
