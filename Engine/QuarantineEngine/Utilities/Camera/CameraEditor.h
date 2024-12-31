#pragma once
#ifndef CAMERA_EDITOR_H
#define CAMERA_EDITOR_H

#include <Camera.h>

class CameraEditor : public Camera
{
public:
    static CameraEditor* instance;

public:
    CameraEditor(float width, float height);
    static CameraEditor* getInstance(float width = 1280.0f, float height = 720.0f);
};

#endif
