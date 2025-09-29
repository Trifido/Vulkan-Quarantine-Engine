#pragma once
#ifndef QE_PROJECT_MANAGER
#define QE_PROJECT_MANAGER

#include <filesystem>
#include <string>
#include <QEScene.h>

const static std::string PROJECTS_FOLDER_PATH = "../../QEProjects";
const static std::string SCENE_FOLDER = "QEScenes";
const static std::string ASSETS_FOLDER = "QEAssets";
const static std::string MODELS_FOLDER = "QEModels";
const static std::string MATERIALS_FOLDER = "QEMaterials";
const static std::string MESH_FOLDER = "Meshes";
const static std::string TEXTURE_FOLDER = "Textures";
const static std::string MATERIAL_FOLDER = "Materials";
const static std::string ANIMATION_FOLDER = "Animations";

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
    static bool CreateYamlScene(const std::string& sceneName = "DefaultScene");
    static bool ImportMeshFile(const fs::path& inputFile);
    static bool ImportAnimationFile(const fs::path& inputFile, const fs::path& folderPath);
    static fs::path GetMaterialFolderPath();

    static bool InitializeDefaultQEScene(QEScene& scene);
};

#endif // !QE_PROJECT_MANAGER
