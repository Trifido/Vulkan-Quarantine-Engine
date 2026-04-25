#pragma once
#ifndef QE_PROJECT_MANAGER
#define QE_PROJECT_MANAGER

#include <filesystem>
#include <string>
#include <functional>
#include <QEScene.h>

using QEImportProgressCallback = std::function<void(float, const std::string&, const std::string&)>;

const static std::string PROJECTS_FOLDER_PATH = "../../QEProjects";
const static std::string SCENE_TEMPLATE = "../../src/QuarantineEngine/Data/SceneTemplates/QETemplate.qescene";
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

    static bool IsInsideCurrentProject(const fs::path& path);

public:
    static bool HasCurrentProject();
    static bool CreateQEProject(const std::string& projectName);
    static bool CreateFolder(const fs::path& projectPath, const std::string& folderName);
    static bool DeletePath(const fs::path& targetPath, bool recursive = true);
    static bool RenamePath(const fs::path& sourcePath, const std::string& newName);
    static bool CreateYamlScene(const std::string& sceneName = "DefaultScene");
    static bool ImportMeshFile(
        const fs::path& inputFile,
        const fs::path& targetFolder,
        const QEImportProgressCallback& onProgress = nullptr);
    static bool ImportTextureFile(
        const fs::path& inputFile,
        const fs::path& targetFolder,
        const QEImportProgressCallback& onProgress = nullptr);
    static bool ImportAnimationFile(const fs::path& inputFile, const fs::path& folderPath);
    static fs::path GetMaterialFolderPath();

    static fs::path GetCurrentProjectPath();
    static fs::path GetCurrentDefaultScenePath();
    static fs::path GetScenesFolderPath();
    static fs::path GetAssetsFolderPath();
    static fs::path GetModelsFolderPath();

    static fs::path ResolveProjectPath(const fs::path& path);
    static std::string ToProjectRelativePath(const fs::path& path);

    static bool InitializeDefaultQEScene(QEScene& scene);
    static bool InitializeQEScene(QEScene& scene, const fs::path& sceneName);

    static bool CreateUniqueFolderAt(
        const fs::path& parentFolderPath,
        const std::string& baseFolderName = "New Folder");

private:
    static bool CreateDefaultProjectMaterials();
};



namespace QE
{
    using ::QEProjectManager;
} // namespace QE
// QE namespace aliases
#endif // !QE_PROJECT_MANAGER
