#pragma once
#ifndef QE_SCENE_H
#define QE_SCENE_H

#include <vector>

#include <filesystem>
#include <AtmosphereDto.h>
#include <CameraDto.h>
#include <GameObjectDto.h>

namespace fs = std::filesystem;
class QEScene
{
private:
    fs::path scenePath;
public:
    std::string sceneName;
    CameraDto cameraEditor;
    AtmosphereDto atmosphere;
    std::vector<GameObjectDto> gameObjectDtos;

public:
    bool InitScene(fs::path filename);
    bool SaveScene();
};

#endif // !QESCENE_H


