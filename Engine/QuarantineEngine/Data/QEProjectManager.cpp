#include "QEProjectManager.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <CameraDto.h>

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
        CURRENT_DEFAULT_SCENE_PATH = scenePath / (sceneName + ".qescene");

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
                size_t sceneNameLength = sceneName.length();
                file.write(reinterpret_cast<const char*>(&sceneNameLength), sizeof(sceneNameLength));
                file.write(sceneName.c_str(), sceneNameLength);

                CameraDto camera;
                file.write(reinterpret_cast<const char*>(&camera.position), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.front), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.up), sizeof(glm::vec3));
                file.write(reinterpret_cast<const char*>(&camera.nearPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.farPlane), sizeof(float));
                file.write(reinterpret_cast<const char*>(&camera.fov), sizeof(float));
                file.close();
                return true;
            }
        }
    }

    return false;
}

bool QEProjectManager::InitializeDefaultQEScene(QEScene& scene)
{
    if (fs::exists(CURRENT_DEFAULT_SCENE_PATH))
    {
        return scene.InitScene(CURRENT_DEFAULT_SCENE_PATH);
    }

    return false;
}
