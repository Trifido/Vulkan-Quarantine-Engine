#include "QECamera.h"
#include <FrustumComponent.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Timer.h>
#include <QESessionManager.h>
#include "QEGameObject.h"

float glm_vec3_dot(glm::vec3 a, glm::vec3 b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float glm_vec3_norm2(glm::vec3 v) {
    return glm_vec3_dot(v, v);
}

float glm_vec3_norm(glm::vec3 v) {
    return sqrtf(glm_vec3_norm2(v));
}

QECamera::QECamera()
{
    QEGameComponent::QEGameComponent();

    this->_near = 0.1f;
    this->_far = 500.0f;
    this->_fov = 45.0f;

    auto swapchainModule = SwapChainModule::getInstance();
    UpdateViewportSize(swapchainModule->swapChainExtent);
}

QECamera::QECamera(const float width, const float height)
{
    QEGameComponent::QEGameComponent();
    this->Width = width;
    this->Height = height;

    this->_near = 0.1f;
    this->_far = 500.0f;
    this->_fov = 45.0f;

    VkExtent2D size =
    {
        .width = (uint32_t)width,
        .height = (uint32_t)height
    };

    UpdateViewportSize(size);
}

void QECamera::UpdateViewportSize(VkExtent2D size)
{
    this->Width = (float)size.width;
    this->Height = (float)size.height;
    _dirtyData = true;
}

void QECamera::UpdateCamera()
{
    glm::vec3 pos = _OwnerTransform ? _OwnerTransform->GetWorldPosition() : glm::vec3(0.0f);
    glm::quat rot = _OwnerTransform ? _OwnerTransform->GetWorldRotation() : glm::quat(1, 0, 0, 0);

    // 2) View = R^T * T^-1   (conjugado del quat = R^T)
    glm::mat4 Rinv = glm::toMat4(glm::conjugate(rot));
    glm::mat4 Tinv = glm::translate(glm::mat4(1.0f), -pos);
    CameraData->View = Rinv * Tinv;

    // 3) Proyección (Vulkan: flip Y)
    UpdateProjectionIfNeeded();

    // 4) Derivados
    CameraData->WPosition = glm::vec4(pos, 1.0f);
    CameraData->Viewproj = CameraData->Projection * CameraData->View;

    UpdateFrustumPlanes();
    if (_frustumComponent) _frustumComponent->ActivateComputeCulling(true);
}

glm::vec4 QECamera::normalize_plane(glm::vec4 p)
{
    glm::vec3 n(p.x, p.y, p.z);
    float len = glm::length(n);
    if (len < 1e-6f) return p;      // avoid NaN
    return p / len;
}

void QECamera::UpdateProjectionIfNeeded()
{
    if (_projCache.fov == _fov && _projCache.n == _near && _projCache.f == _far &&
        _projCache.w == Width && _projCache.h == Height) return;

    const float aspect = glm::max(0.0001f, Width / glm::max(Height, 0.0001f));
    CameraData->Projection = glm::perspective(glm::radians(_fov), aspect, _near, _far);
    CameraData->Projection[1][1] *= -1.0f;
    _projCache = { _fov, _near, _far, Width, Height };
}

void QECamera::UpdateFrustumPlanes()
{
    glm::mat4 viewprojectionTranspose = glm::transpose(this->CameraData->Viewproj);
    this->CameraData->FrustumPlanes[0] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[0]);
    this->CameraData->FrustumPlanes[1] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[0]);
    this->CameraData->FrustumPlanes[2] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[1]);
    this->CameraData->FrustumPlanes[3] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[1]);
    this->CameraData->FrustumPlanes[4] = normalize_plane(viewprojectionTranspose[3] + viewprojectionTranspose[2]);
    this->CameraData->FrustumPlanes[5] = normalize_plane(viewprojectionTranspose[3] - viewprojectionTranspose[2]);

    this->_frustumComponent->RecreateFrustum(this->CameraData->Viewproj);
}

void QECamera::SetNear(float newValue)
{
    _dirtyData = true;
    _near = newValue;
}

void QECamera::SetFar(float newValue)
{
    _dirtyData = true;
    _far = newValue;
}

void QECamera::SetFOV(float newValue)
{
    _dirtyData = true;
    _fov = newValue;
}

void QECamera::QEStart()
{
    if (_QEStarted) return;

    auto sessionManager = QESessionManager::getInstance();
    if (!sessionManager->IsEditor() && this->Owner->Name == sessionManager->NameCameraEditor) return;

    QEGameComponent::QEStart();

    sessionManager->SetFindNewSceneCamera(this->id);

    this->deviceModule = DeviceModule::getInstance();
    this->_frustumComponent = std::make_shared<FrustumComponent>();
    this->CameraData = std::make_shared<UniformCamera>();
}

void QECamera::QEInit()
{
    if (!_QEStarted || _QEInitialized) return;

    QEGameComponent::QEInit();

    _OwnerTransform = this->Owner->GetComponent<QETransform>();
    this->UpdateCamera();
}

void QECamera::QEUpdate()
{
    if (!_QEInitialized) return;

    bool transformChanged = false;

    if (_OwnerTransform)
    {
        uint32_t v = _OwnerTransform->GetWorldVersion();
        if (v != _lastTransformVersion)
        {
            _lastTransformVersion = v;
            transformChanged = true;
        }
    }

    if (_dirtyData || transformChanged)
    {
        UpdateCamera();
        _dirtyData = false;
    }
}

void QECamera::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

