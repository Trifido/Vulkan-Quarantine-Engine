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
    AtmosphereDto atmosphereDto;
    float physicsGravity = -20.0f;

public:
    QEScene();
    QEScene(string sceneName, fs::path scenePath);
    ~QEScene();
    bool InitScene(fs::path filename);
    bool SerializeScene();
    bool DeserializeScene();
    fs::path GetSceneFilePath() const;
    fs::path GetSceneDirectoryPath() const;
};



namespace QE
{
    using ::QEScene;
} // namespace QE
// QE namespace aliases
#endif
