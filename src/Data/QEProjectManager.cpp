#include "QEProjectManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <AtmosphereDto.h>
#include <MeshImporter.h>

fs::path QEProjectManager::CURRENT_PROJECT_PATH;
fs::path QEProjectManager::CURRENT_DEFAULT_SCENE_PATH;

bool QEProjectManager::CreateQEProject(const std::string& projectName)
{
    fs::path folderName = projectName;
    fs::path projectPath = PROJECTS_FOLDER_PATH / folderName;

    CURRENT_PROJECT_PATH = projectPath;

    std::vector<bool> results =
    {
        // Create main folder
        CreateFolder(PROJECTS_FOLDER_PATH, projectName),
        // Create scene folder
        CreateFolder(projectPath, SCENE_FOLDER),
        // Create assets folder
        CreateFolder(projectPath, ASSETS_FOLDER),
        // Create mesh folder
        CreateFolder(projectPath, ASSETS_FOLDER + "/" + MODELS_FOLDER),
        // Create materials folder
        CreateFolder(projectPath, ASSETS_FOLDER + "/" + MATERIALS_FOLDER),
        // Create scene
        CreateYamlScene("default")
    };

    return std::all_of(results.begin(), results.end(), [](bool r) { return r; });
}

bool QEProjectManager::CreateFolder(const fs::path& projectPath, const std::string& folderName)
{
    fs::path folderPath = projectPath / folderName;

    if (fs::exists(projectPath) && !fs::exists(folderPath))
    {
        if (fs::create_directories(folderPath))
        {
            return true;
        }
    }

    printf("Folder: %s already exists\n", folderName.c_str());

    return false;
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
