#include "QEProjectManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <AtmosphereDto.h>
#include <MeshImporter.h>
#include "QEMaterialFileHelper.h"
#include "QEMaterialYamlHelper.h"

fs::path QEProjectManager::CURRENT_PROJECT_PATH;
fs::path QEProjectManager::CURRENT_DEFAULT_SCENE_PATH;

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
        std::cerr << "Error creating folder: " << folderPath << " -> " << ec.message() << std::endl;
        return false;
    }

    return fs::exists(folderPath);
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

bool QEProjectManager::ImportMeshFile(const fs::path& inputFile)
{
    if (!fs::exists(inputFile))
    {
        std::cerr << "Error al abrir el archivo: " << inputFile << std::endl;
        return false;
    }

    string filename = inputFile.filename().string();

    fs::path folderName = inputFile.parent_path().filename();
    fs::path modelFolderPath = CURRENT_PROJECT_PATH / ASSETS_FOLDER / MODELS_FOLDER / folderName;

    CreateFolder(CURRENT_PROJECT_PATH / ASSETS_FOLDER / MODELS_FOLDER, folderName.string());
    CreateFolder(modelFolderPath, MESH_FOLDER);
    CreateFolder(modelFolderPath, TEXTURE_FOLDER);
    CreateFolder(modelFolderPath, MATERIAL_FOLDER);
    CreateFolder(modelFolderPath, ANIMATION_FOLDER);

    fs::path outputMaterialFolderPath = modelFolderPath / MATERIAL_FOLDER;
    fs::path outputTextureFolderPath = modelFolderPath / TEXTURE_FOLDER;
    fs::path outputMeshPath = modelFolderPath / MESH_FOLDER / filename;
    fs::path outputAnimationFolderPath = modelFolderPath / ANIMATION_FOLDER;

    outputMeshPath.replace_extension(".gltf");

    return MeshImporter::LoadAndExportModel(
        inputFile.string(),
        outputMeshPath.string(),
        outputMaterialFolderPath.string(),
        outputTextureFolderPath.string(),
        outputAnimationFolderPath.string()
    );
}

bool QEProjectManager::ImportAnimationFile(const fs::path& inputFile, const fs::path& folderPath)
{
    if (!fs::exists(inputFile))
    {
        std::cerr << "Error al abrir el archivo: " << inputFile << std::endl;
        return false;
    }

    if (!fs::exists(folderPath))
    {
        std::cerr << "Error al abrir el archivo: " << folderPath << std::endl;
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
