#include "QEScene.h"
#include <GameObjectManager.h>
#include <QESessionManager.h>

namespace
{
    constexpr float kDefaultSceneGravity = -20.0f;
}

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
        QE_LOG_ERROR_CAT_F("QEScene", "Error opening the scene {}", scenefile.string());
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
    root["PhysicsSettings"]["Gravity"] = physicsGravity;
    root["Materials"] = materialManager->SerializeMaterials();
    root["GameObjects"] = gameObjectManager->SerializeGameObjects();

    fs::path filePath = this->scenePath / this->sceneName;

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        QE_LOG_ERROR_CAT_F("QEScene", "Error saving the scene {}", this->sceneName);
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

    physicsGravity = kDefaultSceneGravity;

    namespace fs = std::filesystem;
    const fs::path filePath = this->scenePath / this->sceneName;

    YAML::Node root;
    try
    {
        root = YAML::LoadFile(filePath.string());
    }
    catch (const YAML::BadFile& e)
    {
        QE_LOG_ERROR_CAT_F("QEScene", "Error opening the scene {}", filePath.string());
        return false;
    }
    catch (const YAML::ParserException& e)
    {
        QE_LOG_ERROR_CAT_F("QEScene", "Invalid YAML {} ({})", filePath.string(), e.what());
        return false;
    }

    // AtmosphereDto
    if (auto n = root["AtmosphereDto"])
    {
        if (!DeserializeAtmosphere(n, atmosphereDto))
        {
            QE_LOG_WARN_CAT("QEScene", "'AtmosphereDto' could not be deserialised. Using defaults.");
        }
    }
    else
    {
        QE_LOG_WARN_CAT("QEScene", "'AtmosphereDto' node not found in YAML. Using defaults.");
    }

    if (auto n = root["PhysicsSettings"])
    {
        if (auto gravityNode = n["Gravity"])
        {
            physicsGravity = gravityNode.as<float>();
        }
        else
        {
            QE_LOG_WARN_CAT("QEScene", "'PhysicsSettings.Gravity' node not found in YAML. Using default gravity.");
        }
    }
    else
    {
        QE_LOG_WARN_CAT("QEScene", "'PhysicsSettings' node not found in YAML. Using default gravity.");
    }

    if (auto n = root["Materials"])
    {
        materialManager->DeserializeMaterials(n);
    }
    else
    {
        QE_LOG_WARN_CAT("QEScene", "'Materials' node not found in YAML.");
    }

    if (auto n = root["GameObjects"])
    {
        gameObjectManager->DeserializeGameObjects(n);
    }
    else
    {
        QE_LOG_WARN_CAT("QEScene", "'GameObjects' node not found in YAML.");
    }

    return true;
}
