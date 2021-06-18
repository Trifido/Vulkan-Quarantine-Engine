#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float width, float height)
{
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraPos = glm::vec3(0.0f, 15.0f, 15.0f);
    WIDTH = width;
    HEIGHT = height;
    lastX = WIDTH / 2.0f;
    lastY = HEIGHT / 2.0f;
    nearPlane = 0.1f;
    farPlane = 500.0f;
    view = projection = VP = glm::mat4(1.0);
}

void Camera::CameraController(float deltaTime)
{
    EditorScroll();
    EditorRotate();

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('W')) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('w')))
    {
        printf("LO HE CONSEGUIDO\n");
        cameraPos += cameraSpeed * deltaTime * cameraFront;
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('S')) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('s')))
    {
        cameraPos -= cameraSpeed * deltaTime * cameraFront;
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('A')) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('a')))
    {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime * cameraSpeed;
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('D')) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('d')))
    {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime * cameraSpeed;
    }

    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    projection = glm::perspective(glm::radians(fov), (float)WIDTH / (float)HEIGHT, nearPlane, farPlane);
    VP = view * projection;
}

void Camera::EditorScroll()
{
    if (ImGui::GetIO().MouseWheel != 0.0f && ImGui::GetIO().KeyShift)
    {
        if (fov >= 1.0f && fov <= 45.0f)
            fov -= ImGui::GetIO().MouseWheel;
        if (fov <= 1.0f)
            fov = 1.0f;
        if (fov >= 45.0f)
            fov = 45.0f;
    }
}

void Camera::EditorRotate()
{
    if (ImGui::GetIO().KeyShift && ImGui::IsMouseDown(1))
    {
        if (firstMouse)
        {
            lastX = ImGui::GetIO().MousePos.x;
            lastY = ImGui::GetIO().MousePos.y;
            firstMouse = false;
        }

        float xoffset = ImGui::GetIO().MousePos.x - lastX;
        float yoffset = lastY - ImGui::GetIO().MousePos.y; // reversed since y-coordinates go from bottom to top
        lastX = ImGui::GetIO().MousePos.x;
        lastY = ImGui::GetIO().MousePos.y;

        float sensitivity = 0.1f; // change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFront = glm::normalize(front);
    }
    else
    {
        firstMouse = true;
    }
}

void Camera::CheckCameraAttributes(float* positionCamera, float* frontCamera, float fov, float nearPlane, float farPlane)
{
    bool posEqual = true;
    bool lookAtEqual = true;
    if (positionCamera[0] != cameraPos.x || positionCamera[1] != cameraPos.y || positionCamera[2] != cameraPos.z)
        posEqual = false;
    if (frontCamera[0] != cameraFront.x || frontCamera[1] != cameraFront.y || frontCamera[2] != cameraFront.z)
        lookAtEqual = false;
    if (!posEqual || !lookAtEqual || fov != this->fov || nearPlane != this->nearPlane || farPlane != this->farPlane)
    {
        cameraPos.x = positionCamera[0];
        cameraPos.y = positionCamera[1];
        cameraPos.z = positionCamera[2];
        cameraFront.x = frontCamera[0];
        cameraFront.y = frontCamera[1];
        cameraFront.z = frontCamera[2];
        this->fov = fov;
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        projection = glm::perspective(glm::radians(fov), (float)WIDTH / (float)HEIGHT, nearPlane, farPlane);
        VP = view * projection;
    }
}

void Camera::InvertPitch(float heightPos)
{
    cameraPos.y = cameraPos.y + heightPos;

    pitch = -pitch;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
    //cameraUp = -cameraUp;
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

