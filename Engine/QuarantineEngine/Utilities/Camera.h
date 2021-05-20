#pragma once
#ifndef CAMERA_H
#define CAMERA_H
#define _USE_MATH_DEFINES

#include <glm/glm.hpp>
#include <math.h>
#include <GLFW/glfw3.h>

struct UBO_Camera {    //CAMERA
    glm::vec4 position;
    glm::vec4 right;
    glm::vec4 up;
    glm::vec4 forward;

    uint32_t frameCount;
};

class Camera
{
public:
    UBO_Camera  uboCamera;
    char keyDownIndex[500];

    glm::vec3 cameraPosition;
    float cameraYaw;
    float cameraPitch;

    uint32_t frameCount;
    GLFWwindow* ptr_window;

public:
    Camera();
    void AddWindow(GLFWwindow& ref_window);
    void checkCameraMovement();
};

#endif // !CAMERA_H



