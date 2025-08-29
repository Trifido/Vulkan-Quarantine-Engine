#include "QEScenev2.h"

QEScenev2::QEScenev2(string sceneName, fs::path scenePath)
{
    this->sceneName = sceneName;
    this->scenePath = scenePath;
}

bool QEScenev2::InitScenev2(fs::path filename)
{
    return false;
}

bool QEScenev2::SaveScenev2()
{
    YAML::Node root;
    root["CameraEditor"] = serializeComponent(cameraEditor);

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
