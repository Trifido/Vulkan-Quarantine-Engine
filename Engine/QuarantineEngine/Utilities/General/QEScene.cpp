#include "QEScene.h"
#include <fstream>
#include <iostream>
#include <GameObjectManager.h>
#include <LightManager.h>

bool QEScene::InitScene(fs::path filename)
{
    auto gameObjectManager = GameObjectManager::getInstance();

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error al abrir la escena" << filename << std::endl;
        return false;
    }

    this->scenePath = filename.parent_path();

    // Read scene name length and name
    size_t sceneNameLength;
    file.read(reinterpret_cast<char*>(&sceneNameLength), sizeof(sceneNameLength));
    sceneName.resize(sceneNameLength);
    file.read(&sceneName[0], sceneNameLength);

    // Read camera dto
    file.read(reinterpret_cast<char*>(&cameraEditor), sizeof(CameraDto));

    // Read atmosphere dto
    file.read(reinterpret_cast<char*>(&atmosphere), sizeof(AtmosphereDto));

    // Read game objects dtos
    this->gameObjectDtos = gameObjectManager->GetGameObjectDtos(file);

    // Read lights dtos
    this->lightDtos = LightManager::GetLightDtos(file);

    file.close();
    std::cout << "Archivo leído correctamente: " << filename << std::endl;
}

bool QEScene::SaveScene()
{
    auto gameObjectManager = GameObjectManager::getInstance();
    auto lightManager = LightManager::getInstance();

    std::string filename = this->sceneName;
    fs::path filePath = this->scenePath / filename;

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open())
    {
        std::cerr << "Error al guardar la escena:" << this->sceneName << std::endl;
        return false;
    }

    size_t sceneNameLength = this->sceneName.length();
    file.write(reinterpret_cast<const char*>(&sceneNameLength), sizeof(sceneNameLength));
    file.write(this->sceneName.c_str(), sceneNameLength);

    file.write(reinterpret_cast<const char*>(&this->cameraEditor), sizeof(CameraDto));
    file.write(reinterpret_cast<const char*>(&this->atmosphere), sizeof(AtmosphereDto));

    // Save game objects dtos
    gameObjectManager->SaveGameObjects(file);

    // Save lights dtos
    lightManager->SaveLights(file);

    file.close();

    return true;
}
