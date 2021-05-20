#include "Camera.h"

Camera::Camera()
{
    uboCamera.position = { 0, 0, 0, 1 };
    uboCamera.right = { 1, 0, 0, 1 };
    uboCamera.up = { 0, 1, 0, 1 };
    uboCamera.forward = { 0, 0, 1, 1 };
    uboCamera.frameCount = 0;
}

void Camera::AddWindow(GLFWwindow& ref_window)
{
    ptr_window = &ref_window;
}

void Camera::checkCameraMovement()
{
    int isCameraMoved = 0;

    if (keyDownIndex[GLFW_KEY_W]) {
        cameraPosition[0] += cos(-cameraYaw - (M_PI / 2)) * 0.1f;
        cameraPosition[2] += sin(-cameraYaw - (M_PI / 2)) * 0.1f;
        isCameraMoved = 1;
    }
    if (keyDownIndex[GLFW_KEY_S]) {
        cameraPosition[0] -= cos(-cameraYaw - (M_PI / 2)) * 0.1f;
        cameraPosition[2] -= sin(-cameraYaw - (M_PI / 2)) * 0.1f;
        isCameraMoved = 1;
    }
    if (keyDownIndex[GLFW_KEY_A]) {
        cameraPosition[0] -= cos(-cameraYaw) * 0.1f;
        cameraPosition[2] -= sin(-cameraYaw) * 0.1f;
        isCameraMoved = 1;
    }
    if (keyDownIndex[GLFW_KEY_D]) {
        cameraPosition[0] += cos(-cameraYaw) * 0.1f;
        cameraPosition[2] += sin(-cameraYaw) * 0.1f;
        isCameraMoved = 1;
    }
    if (keyDownIndex[GLFW_KEY_SPACE]) {
        cameraPosition[1] += 0.1f;
        isCameraMoved = 1;
    }
    if (keyDownIndex[GLFW_KEY_LEFT_CONTROL]) {
        cameraPosition[1] -= 0.1f;
        isCameraMoved = 1;
    }

    static double previousMousePositionX;
    static double previousMousePositionY;

    double xPos, yPos;
    glfwGetCursorPos(ptr_window, &xPos, &yPos);

    if (previousMousePositionX != xPos || previousMousePositionY != yPos) {
        double mouseDifferenceX = previousMousePositionX - xPos;
        double mouseDifferenceY = previousMousePositionY - yPos;

        cameraYaw += mouseDifferenceX * 0.0005f;

        previousMousePositionX = xPos;
        previousMousePositionY = yPos;

        isCameraMoved = 1;
    }

    uboCamera.position[0] = cameraPosition[0]; uboCamera.position[1] = cameraPosition[1]; uboCamera.position[2] = cameraPosition[2];

    uboCamera.forward[0] = cosf(cameraPitch) * cosf(-cameraYaw - (M_PI / 2.0));
    uboCamera.forward[1] = sinf(cameraPitch);
    uboCamera.forward[2] = cosf(cameraPitch) * sinf(-cameraYaw - (M_PI / 2.0));

    uboCamera.right[0] = uboCamera.forward[1] * uboCamera.up[2] - uboCamera.forward[2] * uboCamera.up[1];
    uboCamera.right[1] = uboCamera.forward[2] * uboCamera.up[0] - uboCamera.forward[0] * uboCamera.up[2];
    uboCamera.right[2] = uboCamera.forward[0] * uboCamera.up[1] - uboCamera.forward[1] * uboCamera.up[0];

    if (isCameraMoved == 1) {
        uboCamera.frameCount = 0;
    }
    else {
        uboCamera.frameCount += 1;
    }
}
