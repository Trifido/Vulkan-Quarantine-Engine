#pragma once
#ifndef QE_SCENEV2_H
#define QE_SCENEV2_H

#include <Camera/CameraEditor.h>
#include <QEGameObject.h>
#include <vector>

using namespace std;

namespace fs = filesystem;
class QEScenev2
{
private:
    fs::path scenePath;

public:
    string sceneName;
    CameraEditor* cameraEditor;
    //vector<QEGameObject> GameObjects;

public:
    QEScenev2(string sceneName, fs::path scenePath);
    bool InitScenev2(fs::path filename);
    bool SaveScenev2();
};

#endif
