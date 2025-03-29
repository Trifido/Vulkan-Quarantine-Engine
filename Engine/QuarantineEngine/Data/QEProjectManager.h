#pragma once
#ifndef QE_PROJECT_MANAGER
#define QE_PROJECT_MANAGER

#include <filesystem>
#include <string>
#include <QEScene.h>

const static std::string PROJECTS_FOLDER_PATH = "../../QEProjects";
const static std::string SCENE_FOLDER = "QEScenes";

namespace fs = std::filesystem;
class QEProjectManager
{
private:
    static fs::path CURRENT_PROJECT_PATH;
    static fs::path CURRENT_DEFAULT_SCENE_PATH;
public:
    static bool CreateQEProject(const std::string& projectName);
    static bool CreateFolder(const fs::path& projectPath, const std::string& folderName);
    static bool CreateScene(const std::string& sceneName = "DefaultScene");

    static bool InitializeDefaultQEScene(QEScene& scene);
};

#endif // !QE_PROJECT_MANAGER
