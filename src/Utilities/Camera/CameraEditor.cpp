#include "CameraEditor.h"

CameraEditor* CameraEditor::instance = nullptr;

CameraEditor::CameraEditor(float width, float height, CameraDto cameraDto) : QECamera(width, height, cameraDto)
{
    
}

CameraEditor* CameraEditor::getInstance(float width, float height, CameraDto cameraDto)
{
    if (instance == NULL)
        instance = new CameraEditor(width, height, cameraDto);

    return instance;
}
