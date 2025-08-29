#include "QEProjectManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <CameraDto.h>
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
        CreateScene("default")
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

bool QEProjectManager::CreateScene(const std::string& sceneName)
{
    fs::path scenePath = CURRENT_PROJECT_PATH / SCENE_FOLDER;

    if (fs::exists(scenePath))
    {
        std::string filename = sceneName + ".qescene";
        CURRENT_DEFAULT_SCENE_PATH = scenePath / filename;

        if (!fs::exists(CURRENT_DEFAULT_SCENE_PATH))
        {
            std::ofstream file(CURRENT_DEFAULT_SCENE_PATH, std::ios::binary);
            if (!file)
            {
                std::cerr << "Error al abrir el archivo para escritura.\n";
                return false;
            }

            if (file.is_open())
            {
                size_t sceneNameLength = filename.length();
                file.write(reinterpret_cast<const char*>(&sceneNameLength), sizeof(sceneNameLength));
                file.write(filename.c_str(), sceneNameLength);

                CameraDto camera;
                file.write(reinterpret_cast<const char*>(&camera.position), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.front), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.up), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.nearPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.farPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.fov), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.pitchSaved), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.yawSaved), sizeof(float));

                AtmosphereDto atmosphere;
                file.write(reinterpret_cast<const char*>(&atmosphere.hasAtmosphere), sizeof(bool));
                file.write(reinterpret_cast<const char*>(&atmosphere.environmentType), sizeof(int));
                file.write(reinterpret_cast<const char*>(&atmosphere.sunDirection), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&atmosphere.sunIntensity), sizeof(float));

                int numMaterials = 0;
                file.write(reinterpret_cast<const char*>(&numMaterials), sizeof(int));

                int numGameObjects = 0;
                file.write(reinterpret_cast<const char*>(&numGameObjects), sizeof(int));

                int numLights = 0;
                file.write(reinterpret_cast<const char*>(&numLights), sizeof(int));

                file.close();
                return true;
            }
        }
    }

    return false;
}

bool QEProjectManager::CreateYamlSceneFile(const std::string& sceneName)
{
    fs::path scenePath = CURRENT_PROJECT_PATH / SCENE_FOLDER;

    if (fs::exists(scenePath))
    {
        std::string filename = sceneName + ".yaml";
        CURRENT_DEFAULT_SCENE_PATH = scenePath / filename;

        if (!fs::exists(CURRENT_DEFAULT_SCENE_PATH))
        {
            std::ofstream file(CURRENT_DEFAULT_SCENE_PATH, std::ios::binary);
            if (!file)
            {
                std::cerr << "Error al abrir el archivo para escritura.\n";
                return false;
            }

            if (file.is_open())
            {
                size_t sceneNameLength = filename.length();
                file.write(reinterpret_cast<const char*>(&sceneNameLength), sizeof(sceneNameLength));
                file.write(filename.c_str(), sceneNameLength);

                CameraDto camera;
                file.write(reinterpret_cast<const char*>(&camera.position), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.front), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.up), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.nearPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.farPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.fov), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.pitchSaved), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.yawSaved), sizeof(float));

                AtmosphereDto atmosphere;
                file.write(reinterpret_cast<const char*>(&atmosphere.hasAtmosphere), sizeof(bool));
                file.write(reinterpret_cast<const char*>(&atmosphere.environmentType), sizeof(int));
                file.write(reinterpret_cast<const char*>(&atmosphere.sunDirection), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&atmosphere.sunIntensity), sizeof(float));

                int numMaterials = 0;
                file.write(reinterpret_cast<const char*>(&numMaterials), sizeof(int));

                int numGameObjects = 0;
                file.write(reinterpret_cast<const char*>(&numGameObjects), sizeof(int));

                int numLights = 0;
                file.write(reinterpret_cast<const char*>(&numLights), sizeof(int));

                file.close();
                return true;
            }
        }
    }

    return false;
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

    fs::path outputMaterialFolderPath = modelFolderPath / MATERIAL_FOLDER;
    fs::path outputTextureFolderPath = modelFolderPath / TEXTURE_FOLDER;
    fs::path outputMeshPath = modelFolderPath / MESH_FOLDER / filename;

    outputMeshPath.replace_extension(".gltf");

    return MeshImporter::LoadAndExportModel(
        inputFile.string(),
        outputMeshPath.string(),
        outputMaterialFolderPath.string(),
        outputTextureFolderPath.string()
    );
}

fs::path QEProjectManager::GetMaterialFolderPath()
{
    return fs::path(CURRENT_PROJECT_PATH / ASSETS_FOLDER / MATERIALS_FOLDER);
}

bool QEProjectManager::InitializeDefaultQEScene(QEScene& scene)
{
    if (fs::exists(CURRENT_DEFAULT_SCENE_PATH))
    {
        return scene.InitScene(CURRENT_DEFAULT_SCENE_PATH);
    }

    return false;
}
