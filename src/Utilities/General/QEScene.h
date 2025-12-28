#pragma once
#ifndef QE_SCENEV2_H
#define QE_SCENEV2_H

#include <QEGameObject.h>
#include <AtmosphereDto.h>
#include <vector>

using namespace std;

namespace fs = filesystem;

class QEScene
{
private:
    fs::path scenePath;

public:
    string sceneName;
    std::shared_ptr<QECamera> cameraEditor = NULL;
    AtmosphereDto atmosphereDto;

public:
    QEScene();
    QEScene(string sceneName, fs::path scenePath);
    ~QEScene();
    bool InitScene(fs::path filename);
    bool SerializeScene();
    bool DeserializeScene();
};

#endif
