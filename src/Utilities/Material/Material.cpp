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

    //this->materialData = {};
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
    this->materialData.Clearcoat = materialDto.Clearcoat;
    this->materialData.ClearcoatRoughness = materialDto.ClearcoatRoughness;

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
        this->materialData.AddTexture(tp.second, tp.first);
    }

    this->materialData.ApplyDtoPacking(materialDto);
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
    if (!file.is_open())
    {
        std::cerr << "Error al abrir " << materialFilePath << " para escritura.\n";
        return "";
    }

    auto writeString = [&](const std::string& s)
        {
            int len = (int)s.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(int));
            if (len > 0)
                file.write(s.data(), len);
        };

    // 1) Header
    writeString(Name);
    writeString(materialFilePath);

    std::string shaderName = (shader ? shader->shaderNameID : "default");
    writeString(shaderName);

    // 2) Layer
    int renderLayer = (int)layer;
    file.write(reinterpret_cast<const char*>(&renderLayer), sizeof(int));

    // 3) Scalars legacy + PBR + Clearcoat (MISMO ORDEN que el reader)
    file.write(reinterpret_cast<const char*>(&materialData.Opacity), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.BumpScaling), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.Shininess), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.Reflectivity), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.Shininess_Strength), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.Refractivity), sizeof(float));

    file.write(reinterpret_cast<const char*>(&materialData.Metallic), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.Roughness), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.AO), sizeof(float));

    // NUEVO: clearcoat
    file.write(reinterpret_cast<const char*>(&materialData.Clearcoat), sizeof(float));
    file.write(reinterpret_cast<const char*>(&materialData.ClearcoatRoughness), sizeof(float));

    // 4) Colors
    file.write(reinterpret_cast<const char*>(&materialData.Diffuse), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&materialData.Ambient), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&materialData.Specular), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&materialData.Emissive), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&materialData.Transparent), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&materialData.Reflective), sizeof(glm::vec4));

    // 5) Mask + channels (si los guardas en MaterialData)
    file.write(reinterpret_cast<const char*>(&materialData.TexMask), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&materialData.MetallicChan), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&materialData.RoughnessChan), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&materialData.AOChan), sizeof(uint32_t));

    writeString(materialData.diffuseTexturePath);
    writeString(materialData.normalTexturePath);
    writeString(materialData.metallicTexturePath);
    writeString(materialData.roughnessTexturePath);
    writeString(materialData.aoTexturePath);
    writeString(materialData.emissiveTexturePath);
    writeString(materialData.heightTexturePath);
    writeString(materialData.specularTexturePath);

    file.close();
    return materialFilePath;
}

