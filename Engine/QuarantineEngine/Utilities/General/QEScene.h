#pragma once
#ifndef QE_SCENE_H
#define QE_SCENE_H

#include <vector>
#include <GameObject.h>

#include <filesystem>
namespace fs = std::filesystem;
class QEScene
{
public:
    std::string sceneName;
    CameraDto cameraEditor;
    std::vector<GameObject> m_GameObjects;

public:
    bool InitScene(fs::path filename);
    //QEScene();
    //~QEScene();

    //void Init();
    //void Update();
    //void Render();
    //void Destroy();
};

#endif // !QESCENE_H


