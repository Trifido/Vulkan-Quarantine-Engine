#pragma once
#ifndef QE_SCENE_H
#define QE_SCENE_H

#include <vector>
#include <GameObject.h>

#include <filesystem>
#include <AtmosphereDto.h>
namespace fs = std::filesystem;
class QEScene
{
private:
    fs::path scenePath;
public:
    std::string sceneName;
    CameraDto cameraEditor;
    AtmosphereDto atmosphere;
    std::vector<GameObject> m_GameObjects;

public:
    bool InitScene(fs::path filename);
    bool SaveScene();
    //QEScene();
    //~QEScene();

    //void Init();
    //void Update();
    //void Render();
    //void Destroy();
};

#endif // !QESCENE_H


