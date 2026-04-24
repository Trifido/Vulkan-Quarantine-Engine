#include "QEProjectManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <AtmosphereDto.h>
#include <MeshImporter.h>
#include "QEMaterialFileHelper.h"
#include "QEMaterialYamlHelper.h"
#include "QEShaderYamlHelper.h"
#include <MaterialManager.h>
#include <Logging/QELogMacros.h>

fs::path QEProjectManager::CURRENT_PROJECT_PATH;
fs::path QEProjectManager::CURRENT_DEFAULT_SCENE_PATH;

namespace
{
    bool SyncRenamedMaterialAssetFile(const fs::path& targetPath)
    {
        MaterialDto materialDto;
        if (!QEMaterialYamlHelper::ReadMaterialFile(targetPath, materialDto))
            return false;

        materialDto.Name = targetPath.stem().string();
        materialDto.FilePath = QEProjectManager::ToProjectRelativePath(targetPath);

        return QEMaterialYamlHelper::WriteMaterialFile(targetPath, materialDto);
    }

    bool SyncRenamedShaderAssetFile(const fs::path& targetPath)
    {
        QEShaderAsset shaderAsset;
        if (!QEShaderYamlHelper::ReadShaderFile(targetPath, shaderAsset))
            return false;

        shaderAsset.Name = targetPath.stem().string();
        shaderAsset.FilePath = QEProjectManager::ToProjectRelativePath(targetPath);

        return QEShaderYamlHelper::WriteShaderFile(targetPath, shaderAsset);
    }
}

fs::path QEProjectManager::GetCurrentProjectPath()
{
    return CURRENT_PROJECT_PATH;
}

fs::path QEProjectManager::GetCurrentDefaultScenePath()
{
    return CURRENT_DEFAULT_SCENE_PATH;
}

fs::path QEProjectManager::GetScenesFolderPath()
{
    return CURRENT_PROJECT_PATH / SCENE_FOLDER;
}

fs::path QEProjectManager::GetAssetsFolderPath()
{
    return CURRENT_PROJECT_PATH / ASSETS_FOLDER;
}

fs::path QEProjectManager::GetModelsFolderPath()
{
    return CURRENT_PROJECT_PATH / ASSETS_FOLDER / MODELS_FOLDER;
}

bool QEProjectManager::HasCurrentProject()
{
    return !CURRENT_PROJECT_PATH.empty() && fs::exists(CURRENT_PROJECT_PATH);
}

bool QEProjectManager::CreateQEProject(const std::string& projectName)
{
    fs::path folderName = projectName;
    fs::path projectPath = PROJECTS_FOLDER_PATH / folderName;

    std::vector<bool> results =
    {
        CreateFolder(PROJECTS_FOLDER_PATH, projectName),
        CreateFolder(projectPath, SCENE_FOLDER),
        CreateFolder(projectPath, ASSETS_FOLDER),
        CreateFolder(projectPath, ASSETS_FOLDER + "/" + MODELS_FOLDER),
        CreateFolder(projectPath, ASSETS_FOLDER + "/" + MATERIALS_FOLDER)
    };

    const bool foldersOk = std::all_of(results.begin(), results.end(), [](bool r) { return r; });

    if (!foldersOk)
        return false;

    CURRENT_PROJECT_PATH = projectPath;

    if (!CreateYamlScene("default"))
        return false;

    if (!CreateDefaultProjectMaterials())
        return false;

    return true;
}

bool QEProjectManager::CreateFolder(const fs::path& projectPath, const std::string& folderName)
{
    fs::path folderPath = projectPath / folderName;

    std::error_code ec;
    fs::create_directories(folderPath, ec);

    if (ec)
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "Error creating folder : {} -> {}", folderPath.string(), ec.message());
        return false;
    }

    return fs::exists(folderPath);
}

bool QEProjectManager::DeletePath(const fs::path& targetPath, bool recursive)
{
    if (!HasCurrentProject())
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "DeletePath - No current project loaded.");
        return false;
    }

    const fs::path resolvedTarget = ResolveProjectPath(targetPath);

    if (!fs::exists(resolvedTarget))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "DeletePath - Target path does not exist: {}",
            resolvedTarget.string());
        return false;
    }

    if (!IsInsideCurrentProject(resolvedTarget))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "DeletePath - Target path is outside current project: {}",
            resolvedTarget.string());
        return false;
    }

    std::error_code ec;

    const fs::path projectRoot = fs::weakly_canonical(GetCurrentProjectPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path scenesRoot = fs::weakly_canonical(GetScenesFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path assetsRoot = fs::weakly_canonical(GetAssetsFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path modelsRoot = fs::weakly_canonical(GetModelsFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path materialsRoot = fs::weakly_canonical(GetMaterialFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path normalizedTarget = fs::weakly_canonical(resolvedTarget, ec);
    if (ec) return false;

    if (normalizedTarget == projectRoot ||
        normalizedTarget == scenesRoot ||
        normalizedTarget == assetsRoot ||
        normalizedTarget == modelsRoot ||
        normalizedTarget == materialsRoot)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "DeletePath - Cannot delete protected project folder: {}",
            normalizedTarget.string());
        return false;
    }

    std::string extensionLower = resolvedTarget.extension().string();
    std::transform(extensionLower.begin(), extensionLower.end(), extensionLower.begin(), ::tolower);

    ec.clear();

    if (fs::is_directory(resolvedTarget))
    {
        if (recursive)
            fs::remove_all(resolvedTarget, ec);
        else
            fs::remove(resolvedTarget, ec);
    }
    else
    {
        fs::remove(resolvedTarget, ec);
    }

    if (ec)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "DeletePath - Error deleting {} : {}",
            resolvedTarget.string(),
            ec.message());
        return false;
    }

    if (extensionLower == ".qemat")
    {
        MaterialManager::getInstance()->RemoveMaterialAsset(resolvedTarget);
    }

    return true;
}

bool QEProjectManager::RenamePath(const fs::path& sourcePath, const std::string& newName)
{
    if (!HasCurrentProject())
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "RenamePath - No current project loaded.");
        return false;
    }

    if (newName.empty())
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "RenamePath - New name is empty.");
        return false;
    }

    const fs::path resolvedSource = ResolveProjectPath(sourcePath);

    std::error_code ec;

    const fs::path projectRoot = fs::weakly_canonical(GetCurrentProjectPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path scenesRoot = fs::weakly_canonical(GetScenesFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path assetsRoot = fs::weakly_canonical(GetAssetsFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path modelsRoot = fs::weakly_canonical(GetModelsFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path materialsRoot = fs::weakly_canonical(GetMaterialFolderPath(), ec);
    if (ec) return false;

    ec.clear();
    const fs::path normalizedSource = fs::weakly_canonical(resolvedSource, ec);
    if (ec) return false;

    if (normalizedSource == projectRoot ||
        normalizedSource == scenesRoot ||
        normalizedSource == assetsRoot ||
        normalizedSource == modelsRoot ||
        normalizedSource == materialsRoot)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "RenamePath - Cannot rename protected project folder: {}",
            normalizedSource.string());
        return false;
    }

    if (!fs::exists(resolvedSource))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "RenamePath - Source path does not exist: {}",
            resolvedSource.string());
        return false;
    }

    if (!IsInsideCurrentProject(resolvedSource))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "RenamePath - Source path is outside current project: {}",
            resolvedSource.string());
        return false;
    }

    const fs::path targetPath = resolvedSource.parent_path() / newName;

    if (fs::exists(targetPath))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "RenamePath - Target path already exists: {}",
            targetPath.string());
        return false;
    }

    const fs::path sourceExtension = resolvedSource.extension();
    fs::rename(resolvedSource, targetPath, ec);

    if (ec)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "RenamePath - Error renaming {} -> {} : {}",
            resolvedSource.string(),
            targetPath.string(),
            ec.message());
        return false;
    }

    std::string extensionLower = sourceExtension.string();
    std::transform(extensionLower.begin(), extensionLower.end(), extensionLower.begin(), ::tolower);

    if (extensionLower == ".qemat")
    {
        SyncRenamedMaterialAssetFile(targetPath);
        MaterialManager::getInstance()->SyncRenamedMaterialAsset(resolvedSource, targetPath);
    }
    else if (extensionLower == ".qeshader")
    {
        SyncRenamedShaderAssetFile(targetPath);
    }

    return true;
}


bool QEProjectManager::CreateYamlScene(const std::string& sceneName)
{
    const fs::path sceneDir = CURRENT_PROJECT_PATH / SCENE_FOLDER;

    std::error_code ec;
    fs::create_directories(sceneDir, ec);
    if (ec) return false;

    const fs::path dst = sceneDir / (sceneName + ".qescene");
    CURRENT_DEFAULT_SCENE_PATH = dst;

    if (fs::exists(dst))
        return false;

    if (!fs::exists(SCENE_TEMPLATE))
        return false;

    ec.clear();
    fs::copy_file(SCENE_TEMPLATE, dst, fs::copy_options::none, ec);
    if (ec) return false;

    return true;
}

bool QEProjectManager::ImportMeshFile(
    const fs::path& inputFile,
    const fs::path& targetFolder,
    const QEImportProgressCallback& onProgress)
{
    if (!fs::exists(inputFile))
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "ImportMeshFile - Error opening the file: {}", inputFile.string());
        return false;
    }

    const fs::path resolvedTarget = ResolveProjectPath(targetFolder);

    if (!IsInsideCurrentProject(resolvedTarget))
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "ImportMeshFile - Target outside project: {}", resolvedTarget.string());
        return false;
    }

    if (!fs::exists(resolvedTarget) || !fs::is_directory(resolvedTarget))
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "ImportMeshFile - Target folder invalid: {}", resolvedTarget.string());
        return false;
    }

    const std::string filename = inputFile.filename().string();

    fs::path outputMaterialFolderPath = resolvedTarget / MATERIAL_FOLDER;
    fs::path outputTextureFolderPath = resolvedTarget / TEXTURE_FOLDER;
    fs::path outputMeshPath = resolvedTarget / MESH_FOLDER / filename;
    fs::path outputAnimationFolderPath = resolvedTarget / ANIMATION_FOLDER;

    std::error_code ec;
    fs::create_directories(outputMaterialFolderPath, ec);
    ec.clear();
    fs::create_directories(outputTextureFolderPath, ec);
    ec.clear();
    fs::create_directories(outputMeshPath.parent_path(), ec);
    ec.clear();
    fs::create_directories(outputAnimationFolderPath, ec);

    outputMeshPath.replace_extension(".gltf");

    return MeshImporter::LoadAndExportModel(
        inputFile.string(),
        outputMeshPath.string(),
        outputMaterialFolderPath.string(),
        outputTextureFolderPath.string(),
        outputAnimationFolderPath.string(),
        onProgress
    );
}

bool QEProjectManager::ImportAnimationFile(const fs::path& inputFile, const fs::path& folderPath)
{
    if (!fs::exists(inputFile))
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "ImportAnimationFile - Error opening the file: {}", inputFile.string());
        return false;
    }

    if (!fs::exists(folderPath))
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "ImportAnimationFile - Error opening the folder: {}", folderPath.string());
        return false;
    }

    return AnimationImporter::ImportAnimation(inputFile.string(), folderPath.string());
}

fs::path QEProjectManager::GetMaterialFolderPath()
{
    return fs::path(CURRENT_PROJECT_PATH / ASSETS_FOLDER / MATERIALS_FOLDER);
}

bool QEProjectManager::InitializeDefaultQEScene(QEScene& scene)
{
    return InitializeQEScene(scene, CURRENT_DEFAULT_SCENE_PATH);
}

bool QEProjectManager::InitializeQEScene(QEScene& scene, const fs::path& scenePath)
{
    if (fs::exists(scenePath))
    {
        return scene.InitScene(scenePath);
    }
    return false;
}

fs::path QEProjectManager::ResolveProjectPath(const fs::path& path)
{
    if (path.is_absolute())
        return path.lexically_normal();

    return (CURRENT_PROJECT_PATH / path).lexically_normal();
}

std::string QEProjectManager::ToProjectRelativePath(const fs::path& path)
{
    std::error_code ec;
    fs::path relative = fs::relative(path, CURRENT_PROJECT_PATH, ec);

    if (ec)
        return path.lexically_normal().generic_string();

    return relative.lexically_normal().generic_string();
}

bool QEProjectManager::CreateDefaultProjectMaterials()
{
    const fs::path materialFolder = GetMaterialFolderPath();

    MaterialDto primitiveDto = QEMaterialFileHelper::BuildDefaultPrimitiveMaterialDto(CURRENT_PROJECT_PATH);
    MaterialDto particlesDto = QEMaterialFileHelper::BuildDefaultParticlesMaterialDto(CURRENT_PROJECT_PATH);
    MaterialDto debugAABBDto = QEMaterialFileHelper::BuildEditorDebugAABBMaterialDto(CURRENT_PROJECT_PATH);
    MaterialDto debugColliderDto = QEMaterialFileHelper::BuildEditorDebugColliderMaterialDto(CURRENT_PROJECT_PATH);
    MaterialDto gridDto = QEMaterialFileHelper::BuildEditorGridMaterialDto(CURRENT_PROJECT_PATH);

    return
        QEMaterialYamlHelper::WriteMaterialFile(materialFolder / "defaultPrimitiveMat.qemat", primitiveDto) &&
        QEMaterialYamlHelper::WriteMaterialFile(materialFolder / "defaultParticlesMat.qemat", particlesDto) &&
        QEMaterialYamlHelper::WriteMaterialFile(materialFolder / "editorDebugAABB.qemat", debugAABBDto) &&
        QEMaterialYamlHelper::WriteMaterialFile(materialFolder / "editorDebugCollider.qemat", debugColliderDto) &&
        QEMaterialYamlHelper::WriteMaterialFile(materialFolder / "editorGrid.qemat", gridDto);
}


bool QEProjectManager::IsInsideCurrentProject(const fs::path& path)
{
    if (!HasCurrentProject())
        return false;

    std::error_code ec;
    const fs::path normalizedProject = fs::weakly_canonical(CURRENT_PROJECT_PATH, ec);
    if (ec)
        return false;

    ec.clear();
    const fs::path normalizedPath = fs::weakly_canonical(ResolveProjectPath(path), ec);
    if (ec)
        return false;

    auto projectIt = normalizedProject.begin();
    auto pathIt = normalizedPath.begin();

    for (; projectIt != normalizedProject.end() && pathIt != normalizedPath.end(); ++projectIt, ++pathIt)
    {
        if (*projectIt != *pathIt)
            return false;
    }

    return projectIt == normalizedProject.end();
}

bool QEProjectManager::CreateUniqueFolderAt(
    const fs::path& parentFolderPath,
    const std::string& baseFolderName)
{
    if (!HasCurrentProject())
    {
        QE_LOG_ERROR_CAT_F("QEProjectManager", "CreateUniqueFolderAt - No current project loaded.");
        return false;
    }

    const fs::path resolvedParent = ResolveProjectPath(parentFolderPath);

    if (!fs::exists(resolvedParent) || !fs::is_directory(resolvedParent))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "CreateUniqueFolderAt - Parent folder does not exist or is not a directory: {}",
            resolvedParent.string());
        return false;
    }

    if (!IsInsideCurrentProject(resolvedParent))
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "CreateUniqueFolderAt - Parent folder is outside current project: {}",
            resolvedParent.string());
        return false;
    }

    std::string candidateName = baseFolderName;
    fs::path candidatePath = resolvedParent / candidateName;

    int index = 1;
    while (fs::exists(candidatePath))
    {
        candidateName = baseFolderName + " " + std::to_string(index);
        candidatePath = resolvedParent / candidateName;
        ++index;
    }

    std::error_code ec;
    fs::create_directories(candidatePath, ec);

    if (ec)
    {
        QE_LOG_ERROR_CAT_F(
            "QEProjectManager",
            "CreateUniqueFolderAt - Error creating folder: {} -> {}",
            candidatePath.string(),
            ec.message());
        return false;
    }

    return fs::exists(candidatePath);
}
