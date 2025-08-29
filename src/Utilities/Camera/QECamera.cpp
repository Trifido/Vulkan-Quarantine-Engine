#include "QECamera.h"

#include <FrustumComponent.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SynchronizationModule.h>
#include <Timer.h>

float glm_vec3_dot(glm::vec3 a, glm::vec3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float glm_vec3_norm2(glm::vec3 v) {
    return glm_vec3_dot(v, v);
}

float glm_vec3_norm(glm::vec3 v) {
    return sqrtf(glm_vec3_norm2(v));
}

QECamera::QECamera(const float width, const float height, const CameraDto& cameraDto)
{
    QEGameComponent::QEGameComponent();
    this->deviceModule = DeviceModule::getInstance();

    this->frustumComponent = std::make_shared<FrustumComponent>();

    this->cameraUniform = std::make_shared<CameraUniform>();
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->cameraUBO->CreateUniformBuffer(sizeof(CameraUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->LoadCameraDto(width, height, cameraDto);
}

bool QECamera::LoadCameraDto(const float width, const float height, const CameraDto& cameraDto)
{

    this->cameraFront = cameraDto.front;
    this->cameraPos = cameraDto.position;// glm::vec3(0.0f, 10.0f, 10.0f);
    this->cameraUp = cameraDto.up;
    this->nearPlane = cameraDto.nearPlane;
    this->farPlane = cameraDto.farPlane;
    this->fov = cameraDto.fov;

    this->WIDTH = width;
    this->HEIGHT = height;

    this->lastX = WIDTH / 2.0f;
    this->lastY = HEIGHT / 2.0f;

    this->UpdateCamera();
    
    this->pitch = cameraDto.pitchSaved;
    this->yaw = cameraDto.yawSaved;

    return true;
}

CameraDto QECamera::CreateCameraDto()
{
    return
    {
        cameraPos, cameraFront, cameraUp,
        nearPlane, farPlane, fov,
        pitch, yaw
    };
}

void QECamera::CameraController(float deltaTime)
{
    EditorScroll();
    EditorRotate();

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown(ImGuiKey_W)))
    {
        cameraPos += cameraSpeed * deltaTime * cameraFront;
        this->isInputUpdated = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown(ImGuiKey_S)))
    {
        cameraPos -= cameraSpeed * deltaTime * cameraFront;
        this->isInputUpdated = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown(ImGuiKey_A)))
    {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime * cameraSpeed;
        this->isInputUpdated = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) ||
        (ImGui::GetIO().KeyShift && ImGui::IsKeyDown(ImGuiKey_D)))
    {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime * cameraSpeed;
        this->isInputUpdated = true;
    }

    if (this->isInputUpdated)
    {
        this->UpdateCamera();
    }
}

void QECamera::EditorScroll()
{
    if (ImGui::GetIO().MouseWheel != 0.0f && ImGui::GetIO().KeyShift)
    {
        if (fov >= 1.0f && fov <= 45.0f)
            fov -= ImGui::GetIO().MouseWheel;
        if (fov <= 1.0f)
            fov = 1.0f;
        if (fov >= 45.0f)
            fov = 45.0f;

        this->isInputUpdated = true;
    }
}

void QECamera::EditorRotate()
{
    if (ImGui::GetIO().KeyShift && ImGui::IsMouseDown(1))
    {
        ImGuiIO& io = ImGui::GetIO();

        if (firstMouse)
        {
            lastX = io.MousePos.x;
            lastY = io.MousePos.y;
            firstMouse = false;
            return;
        }
        
        float xoffset = io.MousePos.x - lastX;
        float yoffset = lastY - io.MousePos.y; // reversed since y-coordinates go from bottom to top
        lastX = io.MousePos.x;
        lastY = io.MousePos.y;

        float sensitivity = 0.1f; // change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        float yawDegrees = glm::radians(yaw);
        float pitchDegrees = glm::radians(pitch);

        glm::vec3 front;
        front.x = cos(yawDegrees) * cos(pitchDegrees);
        front.y = sin(pitchDegrees);
        front.z = sin(yawDegrees) * cos(pitchDegrees);

        cameraFront = glm::normalize(front);

        this->isInputUpdated = true;
    }
    else
    {
        firstMouse = true;
    }
}

void QECamera::CheckCameraAttributes(float* positionCamera, float* frontCamera, float nfov, float nPlane, float fPlane)
{
    bool posEqual = true;
    bool lookAtEqual = true;
    if (positionCamera[0] != cameraPos.x || positionCamera[1] != cameraPos.y || positionCamera[2] != cameraPos.z)
        posEqual = false;
    if (frontCamera[0] != cameraFront.x || frontCamera[1] != cameraFront.y || frontCamera[2] != cameraFront.z)
        lookAtEqual = false;
    if (!posEqual || !lookAtEqual || nfov != this->fov || nPlane != this->nearPlane || fPlane != this->farPlane)
    {
        this->cameraPos.x = positionCamera[0];
        this->cameraPos.y = positionCamera[1];
        this->cameraPos.z = positionCamera[2];
        this->cameraFront.x = frontCamera[0];
        this->cameraFront.y = frontCamera[1];
        this->cameraFront.z = frontCamera[2];
        this->fov = nfov;
        this->nearPlane = nPlane;
        this->farPlane = fPlane;

        this->view = glm::lookAt(this->cameraPos, this->cameraPos + this->cameraFront, this->cameraUp);
        this->projection = glm::perspective(glm::radians(fov), (float)this->WIDTH / (float)this->HEIGHT, this->nearPlane, this->farPlane);
        this->projection[1][1] *= -1;
        this->VP = this->projection * this->view;

        this->UpdateUniform();
        this->UpdateUBOCamera();
    }
}

void QECamera::InvertPitch(float heightPos)
{
    this->cameraPos.y = cameraPos.y + heightPos;

    this->pitch = -this->pitch;
    glm::vec3 front;
    front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    front.y = sin(glm::radians(this->pitch));
    front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

    this->cameraFront = glm::normalize(front);
    this->view = glm::lookAt(this->cameraPos, this->cameraPos + this->cameraFront, this->cameraUp);

    this->UpdateUniform();
    this->UpdateUBOCamera();
}

void QECamera::UpdateViewportSize(VkExtent2D size)
{
    this->WIDTH = (float)size.width;
    this->HEIGHT = (float)size.height;
}

void QECamera::UpdateCamera()
{
    this->view = glm::lookAt(this->cameraPos, this->cameraPos + this->cameraFront, this->cameraUp);
    this->projection = glm::perspective(glm::radians(fov), (float)this->WIDTH / (float)this->HEIGHT, this->nearPlane, this->farPlane);
    this->projection[1][1] *= -1;
    this->VP = this->projection * this->view;

    this->UpdateUniform();
    this->UpdateUBOCamera();

    this->frustumComponent->ActivateComputeCulling(true);
}

void QECamera::UpdateUniform()
{
    this->cameraUniform->projection = this->projection;
    this->cameraUniform->view = this->view;
    this->cameraUniform->viewproj = this->VP;
    this->cameraUniform->position = glm::vec4(this->cameraPos, 1.0f);

    this->UpdateFrustumPlanes();
}

glm::vec4 QECamera::normalize_plane(glm::vec4 plane) {
    float len = glm_vec3_norm({ plane.x, plane.y, plane.z });

    float value = 1.0f / len;
    plane *= value;
    return plane;
}

void QECamera::UpdateFrustumPlanes()
{
    glm::mat4 viewprojectionTranspose = glm::transpose(this->VP);
    this->cameraUniform->frustumPlanes[0] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[0]);
    this->cameraUniform->frustumPlanes[1] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[0]);
    this->cameraUniform->frustumPlanes[2] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[1]);
    this->cameraUniform->frustumPlanes[3] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[1]);
    this->cameraUniform->frustumPlanes[4] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[2]);
    this->cameraUniform->frustumPlanes[5] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[2]);

    this->frustumComponent->RecreateFrustum(this->VP);
}

void QECamera::UpdateUBOCamera()
{
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame], 0, sizeof(CameraUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->cameraUniform.get()), sizeof(CameraUniform));
        vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame]);
    }
}

void QECamera::QEStart()
{
    QEGameComponent::QEStart();

    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
    cameraSpeed = 10.0f;
    yaw = 0.0f;
    pitch = 0.0f;
    firstMouse = true;
    lastX = 1280.0f / 2.0;
    lastY = 720.0 / 2.0;
    fov = 45.0f;
}

void QECamera::QEInit()
{
    QEGameComponent::QEInit();
}

void QECamera::QEUpdate()
{
    CameraController(Timer::DeltaTime);
}

void QECamera::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

void QECamera::CleanCameraUBO()
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

bool QECamera::IsModified()
{
    return this->isInputUpdated;
}

void QECamera::ResetModifiedField()
{
    this->isInputUpdated = false;
}

