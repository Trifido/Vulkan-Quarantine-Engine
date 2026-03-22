#include "QEGizmoController.h"

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include <QEGameObject.h>
#include <QECamera.h>
#include <QETransform.h>

namespace
{
    ImGuizmo::OPERATION ToImGuizmoOperation(QEGizmoController::Operation operation)
    {
        switch (operation)
        {
        case QEGizmoController::Operation::Translate:
            return ImGuizmo::TRANSLATE;
        case QEGizmoController::Operation::Rotate:
            return ImGuizmo::ROTATE;
        case QEGizmoController::Operation::Scale:
            return ImGuizmo::SCALE;
        case QEGizmoController::Operation::None:
        default:
            return ImGuizmo::TRANSLATE;
        }
    }

    ImGuizmo::MODE ToImGuizmoMode(QEGizmoController::Space space)
    {
        switch (space)
        {
        case QEGizmoController::Space::Local:
            return ImGuizmo::LOCAL;
        case QEGizmoController::Space::World:
        default:
            return ImGuizmo::WORLD;
        }
    }
}

QEGizmoController::QEGizmoController()
{
    _operation = Operation::Translate;
    _space = Space::Local;

    _snapEnabled = false;
    _translationSnap = glm::vec3(1.0f);
    _rotationSnapDegrees = 15.0f;
    _scaleSnap = glm::vec3(0.1f);
}

void QEGizmoController::SetOperation(Operation operation)
{
    _operation = operation;
}

QEGizmoController::Operation QEGizmoController::GetOperation() const
{
    return _operation;
}

void QEGizmoController::SetSpace(Space space)
{
    _space = space;
}

QEGizmoController::Space QEGizmoController::GetSpace() const
{
    return _space;
}

void QEGizmoController::SetSnapEnabled(bool value)
{
    _snapEnabled = value;
}

bool QEGizmoController::IsSnapEnabled() const
{
    return _snapEnabled;
}

void QEGizmoController::SetTranslationSnap(const glm::vec3& value)
{
    _translationSnap = value;
}

void QEGizmoController::SetRotationSnap(const float& degrees)
{
    _rotationSnapDegrees = degrees;
}

void QEGizmoController::SetScaleSnap(const glm::vec3& value)
{
    _scaleSnap = value;
}

bool QEGizmoController::Draw(
    std::shared_ptr<QEGameObject> selectedObject,
    std::shared_ptr<QECamera> camera,
    const glm::vec2& viewportPosition,
    const glm::vec2& viewportSize)
{
    if (selectedObject == nullptr || camera == nullptr)
        return false;

    if (_operation == Operation::None)
        return false;

    auto transform = selectedObject->GetComponent<QETransform>();
    if (transform == nullptr)
        return false;

    if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
        return false;

    return ManipulateTransform(
        selectedObject,
        transform,
        camera,
        viewportPosition,
        viewportSize);
}

bool QEGizmoController::IsUsing() const
{
    return ImGuizmo::IsUsing();
}

bool QEGizmoController::IsOver() const
{
    return ImGuizmo::IsOver();
}

bool QEGizmoController::ManipulateTransform(
    std::shared_ptr<QEGameObject> selectedObject,
    std::shared_ptr<QETransform> transform,
    std::shared_ptr<QECamera> camera,
    const glm::vec2& viewportPosition,
    const glm::vec2& viewportSize)
{
    if (selectedObject == nullptr || transform == nullptr || camera == nullptr)
        return false;

    if (camera->CameraData == nullptr)
        return false;

    ImGuizmo::SetDrawlist(nullptr);
    ImGuizmo::SetRect(
        viewportPosition.x,
        viewportPosition.y,
        viewportSize.x,
        viewportSize.y);

    ImGuizmo::SetOrthographic(false);

    ImGuizmo::OPERATION gizmoOperation = ToImGuizmoOperation(_operation);
    ImGuizmo::MODE gizmoMode = ToImGuizmoMode(_space);

    glm::mat4 worldMatrix = transform->GetWorldMatrix();

    float snapValues[3] = { 0.0f, 0.0f, 0.0f };
    float* snapPtr = nullptr;

    if (_snapEnabled)
    {
        switch (_operation)
        {
        case Operation::Translate:
            snapValues[0] = _translationSnap.x;
            snapValues[1] = _translationSnap.y;
            snapValues[2] = _translationSnap.z;
            snapPtr = snapValues;
            break;

        case Operation::Rotate:
            snapValues[0] = _rotationSnapDegrees;
            snapPtr = snapValues;
            break;

        case Operation::Scale:
            snapValues[0] = _scaleSnap.x;
            snapValues[1] = _scaleSnap.y;
            snapValues[2] = _scaleSnap.z;
            snapPtr = snapValues;
            break;

        case Operation::None:
        default:
            break;
        }
    }

    glm::mat4 gizmoProjection = camera->CameraData->Projection;
    gizmoProjection[1][1] *= -1.0f;

    bool changed = ImGuizmo::Manipulate(
        glm::value_ptr(camera->CameraData->View),
        glm::value_ptr(gizmoProjection),
        gizmoOperation,
        gizmoMode,
        glm::value_ptr(worldMatrix),
        nullptr,
        snapPtr);

    if (!changed)
        return false;

    glm::mat4 localMatrix = worldMatrix;

    auto parentTransform = transform->GetParent();
    if (parentTransform != nullptr)
    {
        const glm::mat4& parentWorld = parentTransform->GetWorldMatrix();
        localMatrix = glm::inverse(parentWorld) * worldMatrix;
    }

    transform->SetFromMatrix(localMatrix);

    return true;
}
