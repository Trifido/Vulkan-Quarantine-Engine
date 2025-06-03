#pragma once
#ifndef CAMERA_EDITOR_H
#define CAMERA_EDITOR_H

#include <QECamera.h>

class CameraEditor : public QECamera
{
public:
    static CameraEditor* instance;

public:
    CameraEditor(float width, float height, CameraDto cameraDto);
    static CameraEditor* getInstance(float width = 1280.0f, float height = 720.0f, CameraDto cameraDto = CameraDto());
};

#endif
