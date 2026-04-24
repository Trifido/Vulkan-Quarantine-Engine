#include "Material.h"
#include <QEProjectManager.h>
#include <QEMaterialYamlHelper.h>
#include <Helpers/ScopedTimer.h>

QEMaterial::QEMaterial(std::string name, std::string filepath)
{
    this->Name = name;

    if (filepath.empty())
    {
        fs::path filenamePath = QEProjectManager::GetMaterialFolderPath();
        filenamePath /= name + ".qemat";
        this->materialFilePath = filenamePath.lexically_normal().string();
    }
    else
    {
        fs::path p(filepath);

        if (p.is_relative())
            p = QEProjectManager::ResolveProjectPath(p);

        this->materialFilePath = p.lexically_normal().string();
    }

    this->renderQueue = static_cast<unsigned int>(this->renderQueue);
    this->lightManager = LightManager::getInstance();
}

QEMaterial::QEMaterial(std::string name, std::shared_ptr<ShaderModule> shader_ptr, std::string filepath) : QEMaterial(name, filepath)
{
    this->shader = shader_ptr;
    this->hasDescriptorBuffer = (shader_ptr != nullptr) && shader_ptr->reflectShader.bindings.size() > 0;

    if (this->hasDescriptorBuffer)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }
}

QEMaterial::QEMaterial(std::shared_ptr<ShaderModule> shader_ptr, const MaterialDto& materialDto)
    : QEMaterial(materialDto.Name, shader_ptr, materialDto.FilePath)
{
    this->renderQueue = materialDto.RenderQueue;

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
    this->materialData.AlphaCutoff = materialDto.AlphaCutoff;
    this->materialData.AlphaMode = materialDto.AlphaMode;
    this->materialData.DoubleSided = materialDto.DoubleSided;

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
    mat_instance->renderQueue = this->renderQueue;
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
    if (this->hasDescriptorBuffer && !this->descriptor)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }

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
    this->IsInitialized = false;
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
    if (this->HasDescriptorBuffer() &&
        this->descriptor &&
        idx < this->descriptor->descriptorSets.size())
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

    if (this->shader->reflectShader.HasSpotShadows)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->shader->PipelineModule->pipelineLayout, 3, 1, &lightManager->SpotShadowDescritors->renderDescriptorSets[idx], 0, nullptr);
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
    fs::path absMaterialPath = QEProjectManager::ResolveProjectPath(materialFilePath);
    this->materialFilePath = absMaterialPath.lexically_normal().string();

    MaterialDto dto = this->ToDto();
    dto.FilePath = QEProjectManager::ToProjectRelativePath(absMaterialPath);

    if (!QEMaterialYamlHelper::WriteMaterialFile(absMaterialPath, dto))
    {
        QE_LOG_ERROR_CAT_F("QEMaterial", "Error saving YAML content to {}", absMaterialPath.string());
        return "";
    }

    return this->materialFilePath;
}

MaterialDto QEMaterial::ToDto() const
{
    MaterialDto dto{};

    fs::path absMaterialPath = QEProjectManager::ResolveProjectPath(this->materialFilePath);

    dto.Name = this->Name;
    dto.FilePath = QEProjectManager::ToProjectRelativePath(absMaterialPath);
    dto.ShaderPath = !this->shaderAssetPath.empty()
        ? this->shaderAssetPath
        : (this->shader ? this->shader->shaderNameID : "default");
    dto.RenderQueue = static_cast<unsigned int>(this->renderQueue);

    dto.Opacity = this->materialData.Opacity;
    dto.BumpScaling = this->materialData.BumpScaling;
    dto.Shininess = this->materialData.Shininess;
    dto.Reflectivity = this->materialData.Reflectivity;
    dto.Shininess_Strength = this->materialData.Shininess_Strength;
    dto.Refractivity = this->materialData.Refractivity;
    dto.Metallic = this->materialData.Metallic;
    dto.Roughness = this->materialData.Roughness;
    dto.AO = this->materialData.AO;
    dto.Clearcoat = this->materialData.Clearcoat;
    dto.ClearcoatRoughness = this->materialData.ClearcoatRoughness;
    dto.AlphaCutoff = this->materialData.AlphaCutoff;

    dto.Diffuse = this->materialData.Diffuse;
    dto.Ambient = this->materialData.Ambient;
    dto.Specular = this->materialData.Specular;
    dto.Emissive = this->materialData.Emissive;
    dto.Transparent = this->materialData.Transparent;
    dto.Reflective = this->materialData.Reflective;

    dto.texMask = this->materialData.TexMask;
    dto.metallicChan = this->materialData.MetallicChan;
    dto.roughnessChan = this->materialData.RoughnessChan;
    dto.aoChan = this->materialData.AOChan;
    dto.AlphaMode = this->materialData.AlphaMode;
    dto.DoubleSided = this->materialData.DoubleSided;

    dto.diffuseTexturePath = ToMaterialRelativePath(this->materialData.diffuseTexturePath, absMaterialPath);
    dto.normalTexturePath = ToMaterialRelativePath(this->materialData.normalTexturePath, absMaterialPath);
    dto.specularTexturePath = ToMaterialRelativePath(this->materialData.specularTexturePath, absMaterialPath);
    dto.emissiveTexturePath = ToMaterialRelativePath(this->materialData.emissiveTexturePath, absMaterialPath);
    dto.heightTexturePath = ToMaterialRelativePath(this->materialData.heightTexturePath, absMaterialPath);
    dto.metallicTexturePath = ToMaterialRelativePath(this->materialData.metallicTexturePath, absMaterialPath);
    dto.roughnessTexturePath = ToMaterialRelativePath(this->materialData.roughnessTexturePath, absMaterialPath);
    dto.aoTexturePath = ToMaterialRelativePath(this->materialData.aoTexturePath, absMaterialPath);

    return dto;
}

bool QEMaterial::ApplyShader(const std::shared_ptr<ShaderModule>& shaderPtr, const std::string& assetPath)
{
    if (!shaderPtr)
        return false;

    if (this->descriptor)
    {
        this->descriptor->Cleanup();
        this->descriptor.reset();
        this->descriptor = nullptr;
    }

    this->materialData.CleanMaterialUBO();
    this->IsInitialized = false;
    this->shader = shaderPtr;
    this->shaderAssetPath = assetPath;
    this->hasDescriptorBuffer = shaderPtr->reflectShader.bindings.size() > 0;

    if (this->hasDescriptorBuffer)
    {
        this->descriptor = std::make_shared<DescriptorBuffer>(this->shader);
    }

    this->InitializeMaterialData();
    return true;
}

std::string QEMaterial::ToMaterialRelativePath(
    const std::string& assetPath,
    const std::filesystem::path& materialFilePath)
{
    if (assetPath.empty())
        return "";

    if (assetPath == "NULL_TEXTURE")
        return "NULL_TEXTURE";

    namespace fs = std::filesystem;

    fs::path p(assetPath);
    fs::path materialDir = materialFilePath.parent_path();

    std::error_code ec;
    fs::path rel = fs::relative(p, materialDir, ec);

    if (ec)
        return p.lexically_normal().string();

    return rel.lexically_normal().string();
}
