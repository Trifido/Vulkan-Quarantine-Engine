#include "CameraEditor.h"

CameraEditor* CameraEditor::instance = nullptr;

CameraEditor::CameraEditor(float width, float height) : Camera(width, height)
{
    
}

CameraEditor* CameraEditor::getInstance(float width, float height)
{
    if (instance == NULL)
        instance = new CameraEditor(width, height);

    return instance;
}
