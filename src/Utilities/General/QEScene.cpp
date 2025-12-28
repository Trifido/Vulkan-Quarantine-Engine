#include "QEScene.h"
#include <GameObjectManager.h>
#include <QESessionManager.h>

QEScene::QEScene()
{
}

QEScene::QEScene(string sceneName, fs::path scenePath) : QEScene()
{
    this->sceneName = sceneName;
    this->scenePath = scenePath;
}

QEScene::~QEScene()
{
    cameraEditor = NULL;
}

bool QEScene::InitScene(fs::path scenefile)
{
    std::ifstream file(scenefile, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error al abrir la escena" << scenefile << std::endl;
        return false;
    }

    this->scenePath = scenefile.parent_path();
    this->sceneName = scenefile.filename().string();

    return true;
}

bool QEScene::SerializeScene()
{
    auto gameObjectManager = GameObjectManager::getInstance();
    auto materialManager = MaterialManager::getInstance();
    auto lightManager = LightManager::getInstance();

    YAML::Node root;
    root["AtmosphereDto"] = SerializeAtmosphere(atmosphereDto);
    root["Materials"] = materialManager->SerializeMaterials();
    root["GameObjects"] = gameObjectManager->SerializeGameObjects();

    fs::path filePath = this->scenePath / this->sceneName;

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        std::cerr << "Error al guardar la escena:" << this->sceneName << std::endl;
        return false;
    }

    file << root;
    file.close();

    return true;
}

bool QEScene::DeserializeScene()
{
    auto gameObjectManager = GameObjectManager::getInstance();
    auto materialManager = MaterialManager::getInstance();

    namespace fs = std::filesystem;
    const fs::path filePath = this->scenePath / this->sceneName;

    YAML::Node root;
    try
    {
        root = YAML::LoadFile(filePath.string());
    }
    catch (const YAML::BadFile& e)
    {
        std::cerr << "No se pudo abrir la escena: " << filePath << " (" << e.what() << ")\n";
        return false;
    }
    catch (const YAML::ParserException& e)
    {
        std::cerr << "YAML inválido en " << filePath << " (" << e.what() << ")\n";
        return false;
    }

    // CameraEditor
    //if (auto n = root["CameraEditor"])
    //{
    //    cameraEditor = QESessionManager::getInstance()->EditorCamera();
    //    deserializeComponent(cameraEditor.get(), n);
    //    cameraEditor->UpdateCamera();
    //}
    //else
    //{
    //    std::cerr << "Warning: nodo 'CameraEditor' no encontrado en YAML.\n";
    //}

    // AtmosphereDto
    if (auto n = root["AtmosphereDto"])
    {
        if (!DeserializeAtmosphere(n, atmosphereDto))
        {
            std::cerr << "Warning: 'AtmosphereDto' no se pudo deserializar. Usando defaults.\n";
        }
    }
    else
    {
        std::cerr << "Warning: nodo 'AtmosphereDto' no encontrado en YAML. Usando defaults.\n";
    }

    if (auto n = root["Materials"])
    {
        materialManager->DeserializeMaterials(n);
    }
    else
    {
        std::cerr << "Warning: nodo 'Materials' no encontrado en YAML.\n";
    }

    if (auto n = root["GameObjects"])
    {
        gameObjectManager->DeserializeGameObjects(n);
    }
    else
    {
        std::cerr << "Warning: nodo 'GameObjects' no encontrado en YAML.\n";
    }

    return true;
}
