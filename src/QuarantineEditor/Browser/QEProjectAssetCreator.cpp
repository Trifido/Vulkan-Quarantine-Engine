#include "QEProjectAssetCreator.h"

#include <QEProjectManager.h>
#include <QEMaterialYamlHelper.h>
#include <QEShaderYamlHelper.h>
#include <MaterialDto.h>
#include <QEShaderAsset.h>
#include <Logging/QELogMacros.h>

namespace
{
    std::string BuildIndexedName(const std::string& baseName, int index)
    {
        if (index <= 0)
            return baseName;

        return baseName + " " + std::to_string(index);
    }
}

bool QEProjectAssetCreator::ValidateParentFolder(const fs::path& parentFolderPath)
{
    if (!QEProjectManager::HasCurrentProject())
    {
        QE_LOG_ERROR_CAT_F("QEProjectAssetCreator", "No current project loaded.");
        return false;
    }

    const fs::path resolvedParent = QEProjectManager::ResolveProjectPath(parentFolderPath);

    if (!fs::exists(resolvedParent) || !fs::is_directory(resolvedParent))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectAssetCreator",
            "Parent folder does not exist or is not a directory: {}",
            resolvedParent.string());
        return false;
    }

    return true;
}

fs::path QEProjectAssetCreator::BuildUniqueFilePath(
    const fs::path& parentFolderPath,
    const std::string& baseName,
    const std::string& extension)
{
    const fs::path resolvedParent = QEProjectManager::ResolveProjectPath(parentFolderPath);

    int index = 0;
    while (true)
    {
        const std::string candidateName = BuildIndexedName(baseName, index);
        const fs::path candidatePath = resolvedParent / (candidateName + extension);

        if (!fs::exists(candidatePath))
            return candidatePath;

        ++index;
    }
}

bool QEProjectAssetCreator::CreateSceneAt(const fs::path& parentFolderPath, const std::string& baseName)
{
    if (!ValidateParentFolder(parentFolderPath))
        return false;

    const fs::path resolvedParent = QEProjectManager::ResolveProjectPath(parentFolderPath);
    const fs::path outputPath = BuildUniqueFilePath(resolvedParent, baseName, ".qescene");

    if (!fs::exists(SCENE_TEMPLATE))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectAssetCreator",
            "Scene template does not exist: {}",
            SCENE_TEMPLATE);
        return false;
    }

    std::error_code ec;
    fs::copy_file(SCENE_TEMPLATE, outputPath, fs::copy_options::none, ec);

    if (ec)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectAssetCreator",
            "Error creating scene file {} -> {}",
            outputPath.string(),
            ec.message());
        return false;
    }

    return fs::exists(outputPath);
}

bool QEProjectAssetCreator::CreateMaterialAt(const fs::path& parentFolderPath, const std::string& baseName)
{
    if (!ValidateParentFolder(parentFolderPath))
        return false;

    const fs::path resolvedParent = QEProjectManager::ResolveProjectPath(parentFolderPath);
    const fs::path outputPath = BuildUniqueFilePath(resolvedParent, baseName, ".qemat");

    MaterialDto dto{};
    dto.Name = outputPath.stem().string();
    dto.FilePath = outputPath.generic_string();
    dto.ShaderPath = "default";
    dto.RenderQueue = static_cast<unsigned int>(RenderQueue::Geometry);

    return QEMaterialYamlHelper::WriteMaterialFile(outputPath, dto);
}

bool QEProjectAssetCreator::CreateShaderAt(const fs::path& parentFolderPath, const std::string& baseName)
{
    if (!ValidateParentFolder(parentFolderPath))
        return false;

    const fs::path resolvedParent = QEProjectManager::ResolveProjectPath(parentFolderPath);
    const fs::path outputPath = BuildUniqueFilePath(resolvedParent, baseName, ".qeshader");

    QEShaderAsset asset{};
    asset.Name = outputPath.stem().string();
    asset.FilePath = outputPath.generic_string();
    asset.EntryPoint = "main";
    asset.PipelineType = QEShaderPipelineType::Graphics;
    asset.Graphics.HasVertexData = true;
    asset.Graphics.VertexStride = 0;
    asset.Graphics.Topology = QEShaderTopology::TriangleList;
    asset.Graphics.PolygonMode = QEShaderPolygonMode::Fill;
    asset.Graphics.CullMode = QEShaderCullMode::Back;
    asset.Graphics.FrontFace = QEShaderFrontFace::CounterClockwise;
    asset.Graphics.DepthTest = true;
    asset.Graphics.DepthWrite = true;
    asset.Graphics.LineWidth = 1.0f;
    asset.Graphics.IsMeshShader = false;
    asset.Shadow.Enabled = false;
    asset.Shadow.Mode = ShadowMappingMode::NONE;

    return QEShaderYamlHelper::WriteShaderFile(outputPath, asset);
}
