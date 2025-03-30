#include "QEScene.h"
#include <fstream>

bool QEScene::InitScene(fs::path filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error al abrir la escena" << filename << std::endl;
        return false;
    }

    this->scenePath = filename.parent_path();

    // Leer el nombre de la escena
    size_t sceneNameLength;
    file.read(reinterpret_cast<char*>(&sceneNameLength), sizeof(sceneNameLength));
    sceneName.resize(sceneNameLength);
    file.read(&sceneName[0], sceneNameLength);

    // Leer los datos de la c�mara
    file.read(reinterpret_cast<char*>(&cameraEditor), sizeof(CameraDto));

    // Leer los datos de la atm�sfera
    file.read(reinterpret_cast<char*>(&atmosphere), sizeof(AtmosphereDto));

    file.close();
    std::cout << "Archivo le�do correctamente: " << filename << std::endl;
}

bool QEScene::SaveScene()
{
    std::string filename = this->sceneName;
    fs::path filePath = this->scenePath / filename;

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
        std::cerr << "Error al guardar la escena:" << this->sceneName << std::endl;
        return false;
    }

    size_t sceneNameLength = this->sceneName.length();
    file.write(reinterpret_cast<const char*>(&sceneNameLength), sizeof(sceneNameLength));
    file.write(this->sceneName.c_str(), sceneNameLength);

    file.write(reinterpret_cast<const char*>(&this->cameraEditor), sizeof(CameraDto));
    file.write(reinterpret_cast<const char*>(&this->atmosphere), sizeof(AtmosphereDto));

    file.close();

    return true;
}
