#include "QEScene.h"
#include <fstream>

bool QEScene::InitScene(fs::path filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error al abrir la escena" << filename << std::endl;
        return false;
    }

    // Leer el nombre de la escena
    size_t sceneNameLength;
    file.read(reinterpret_cast<char*>(&sceneNameLength), sizeof(sceneNameLength));
    sceneName.resize(sceneNameLength);
    file.read(&sceneName[0], sceneNameLength);

    // Leer los datos de la cámara
    file.read(reinterpret_cast<char*>(&cameraEditor), sizeof(CameraDto));

    file.close();
    std::cout << "Archivo leído correctamente: " << filename << std::endl;
}
