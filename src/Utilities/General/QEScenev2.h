#pragma once
#ifndef QE_SCENEV2_H
#define QE_SCENEV2_H

#include <Camera/CameraEditor.h>
#include <QEGameObject.h>
#include <AtmosphereDto.h>
#include <vector>
#include <GameObjectManager.h>

using namespace std;

namespace fs = filesystem;

class GameObjectManager;

class QEScenev2
{
private:
    fs::path scenePath;

public:
    string sceneName;
    QECamera* cameraEditor = NULL;
    AtmosphereDto atmosphereDto;
    //vector<QEGameObject> GameObjects;

public:
    QEScenev2();
    QEScenev2(string sceneName, fs::path scenePath);
    ~QEScenev2();
    bool InitScenev2(fs::path filename);
    bool SerializeScene(const YAML::Node &gameObjects);
    bool DeserializeScene(GameObjectManager* gameobjectManager);
};

#endif
