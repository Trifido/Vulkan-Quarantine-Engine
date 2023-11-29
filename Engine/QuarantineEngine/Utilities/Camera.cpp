#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SynchronizationModule.h>

float glm_vec3_dot(glm::vec3 a, glm::vec3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float glm_vec3_norm2(glm::vec3 v) {
    return glm_vec3_dot(v, v);
}

float glm_vec3_norm(glm::vec3 v) {
    return sqrtf(glm_vec3_norm2(v));
}

Camera::Camera(float width, float height)
{
    deviceModule = DeviceModule::getInstance();
    cameraFront = glm::vec3(0.815122545f, -0.579281569f, 0.0f);
    cameraPos = glm::vec3(-5.0f, 5.0f, 0.0f);
    WIDTH = width;
    HEIGHT = height;
    lastX = WIDTH / 2.0f;
    lastY = HEIGHT / 2.0f;
    nearPlane = 0.01f;
    farPlane = 100.0f;
    view = projection = VP = glm::mat4(1.0);
    this->cameraUniform = std::make_shared<CameraUniform>();
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->cameraUBO->CreateUniformBuffer(sizeof(CameraUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->UpdateUniform();
    this->UpdateUBOCamera();

    float value = asin(-cameraFront.y);
    float degreeValue = glm::degrees(value);
    if (degreeValue < 0) degreeValue += 180;
    pitch = -degreeValue;

    value = atan2(cameraFront.x, cameraFront.z);
    degreeValue = glm::degrees(value);
    if (degreeValue < 0) degreeValue += 180;
    yaw = (270 + (int)degreeValue) % 360;

}

void Camera::CameraController(float deltaTime)
{
    EditorScroll();
    EditorRotate();

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('W')) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown('w')))
    {
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
    projection[1][1] *= -1;
    VP = projection * view;

    this->UpdateUniform();
    this->UpdateUBOCamera();
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

        float yawDegrees = glm::radians(yaw);
        float pitchDegrees = glm::radians(pitch);

        glm::vec3 front;
        front.x = cos(yawDegrees) * cos(pitchDegrees);
        front.y = sin(pitchDegrees);
        front.z = sin(yawDegrees) * cos(pitchDegrees);

        cameraFront = glm::normalize(front);
    }
    else
    {
        firstMouse = true;
    }
}

void Camera::CheckCameraAttributes(float* positionCamera, float* frontCamera, float nfov, float nPlane, float fPlane)
{
    bool posEqual = true;
    bool lookAtEqual = true;
    if (positionCamera[0] != cameraPos.x || positionCamera[1] != cameraPos.y || positionCamera[2] != cameraPos.z)
        posEqual = false;
    if (frontCamera[0] != cameraFront.x || frontCamera[1] != cameraFront.y || frontCamera[2] != cameraFront.z)
        lookAtEqual = false;
    if (!posEqual || !lookAtEqual || nfov != this->fov || nPlane != this->nearPlane || fPlane != this->farPlane)
    {
        cameraPos.x = positionCamera[0];
        cameraPos.y = positionCamera[1];
        cameraPos.z = positionCamera[2];
        cameraFront.x = frontCamera[0];
        cameraFront.y = frontCamera[1];
        cameraFront.z = frontCamera[2];
        this->fov = nfov;
        this->nearPlane = nPlane;
        this->farPlane = fPlane;

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        projection = glm::perspective(glm::radians(fov), (float)WIDTH / (float)HEIGHT, nearPlane, farPlane);
        projection[1][1] *= -1;
        VP = projection * view;

        this->UpdateUniform();
        this->UpdateUBOCamera();
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

    this->UpdateUniform();
    this->UpdateUBOCamera();
}

void Camera::UpdateSize(VkExtent2D size)
{
    this->WIDTH = size.width;
    this->HEIGHT = size.height;
}

void Camera::UpdateUniform()
{
    this->cameraUniform->projection = this->projection;
    this->cameraUniform->view = this->view;
    this->cameraUniform->viewproj = this->projection * this->view;
    this->cameraUniform->position = glm::vec4(this->cameraPos, 1.0f);

    this->UpdateFrustumPlanes();
}

glm::vec4 Camera::normalize_plane(glm::vec4 plane) {
    float len = glm_vec3_norm({ plane.x, plane.y, plane.z });

    float value = 1.0f / len;
    plane *= value;
    return plane;
}

void Camera::UpdateFrustumPlanes()
{
    glm::mat4 projectionTranspose = glm::transpose(this->projection);
    this->cameraUniform->frustumPlanes[0] = normalize_plane(projectionTranspose[3] + projectionTranspose[0]);
    this->cameraUniform->frustumPlanes[1] = normalize_plane(projectionTranspose[3] - projectionTranspose[0]);
    this->cameraUniform->frustumPlanes[2] = normalize_plane(projectionTranspose[3] + projectionTranspose[1]);
    this->cameraUniform->frustumPlanes[3] = normalize_plane(projectionTranspose[3] - projectionTranspose[1]);
    this->cameraUniform->frustumPlanes[4] = normalize_plane(projectionTranspose[3] + projectionTranspose[2]);
    this->cameraUniform->frustumPlanes[5] = normalize_plane(projectionTranspose[3] - projectionTranspose[2]);
}

void Camera::UpdateUBOCamera()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    void* data;
    vkMapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame], 0, sizeof(CameraUniform), 0, &data);
    memcpy(data, static_cast<const void*>(this->cameraUniform.get()), sizeof(CameraUniform));
    vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame]);
}

void Camera::CleanCameraUBO()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->cameraUniform != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->cameraUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], nullptr);
        }
    }
}

