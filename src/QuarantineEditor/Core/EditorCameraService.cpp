#include "EditorCameraService.h"

#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Core/QEEditorCameraController.h>
#include <QECamera.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include <GameObjectManager.h>
#include <imgui.h>

namespace
{
    constexpr const char* kEditorCameraName = "QECameraEditor";
}

void EditorCameraService::EnsureEditorCamera(GameObjectManager* gameObjectManager)
{
    if (!gameObjectManager)
        return;

    auto cameraObject = gameObjectManager->GetGameObject(kEditorCameraName);
    if (!cameraObject)
    {
        cameraObject = std::make_shared<QEGameObject>(kEditorCameraName);
        cameraObject->AddComponent(std::make_shared<QECamera>(1280.0f, 720.0f));
        cameraObject->AddComponent(std::make_shared<QEEditorCameraController>());

        auto cameraTransform = cameraObject->GetComponent<QETransform>();
        if (cameraTransform)
        {
            cameraTransform->SetLocalEulerDegrees(glm::vec3(-45.0f, 0.0f, 0.0f));
            cameraTransform->SetLocalPosition(glm::vec3(0.0f, 10.0f, 10.0f));
        }

        gameObjectManager->AddGameObject(cameraObject);
    }

    _editorCamera = cameraObject->GetComponent<QECamera>();
}

void EditorCameraService::UpdateInputState(EditorContext* editorContext) const
{
    if (!_editorCamera || !_editorCamera->Owner)
        return;

    auto controller = _editorCamera->Owner->GetComponent<QEEditorCameraController>();
    if (!controller)
        return;

    const bool popupOpen =
        ImGui::IsPopupOpen("AddComponentPopup", ImGuiPopupFlags_AnyPopupId) ||
        ImGui::IsPopupOpen("Rename GameObject", ImGuiPopupFlags_AnyPopupId);

    const bool allowInput =
        editorContext &&
        !popupOpen &&
        !editorContext->BlockViewportInput &&
        (editorContext->ViewportFocused || editorContext->ViewportImageHovered);

    controller->SetInputEnabled(allowInput);
}

glm::vec3 EditorCameraService::GetSpawnPositionInFront(float distance) const
{
    if (!_editorCamera || !_editorCamera->Owner)
        return glm::vec3(0.0f);

    auto cameraTransform = _editorCamera->Owner->GetComponent<QETransform>();
    if (!cameraTransform)
        return glm::vec3(0.0f);

    const glm::vec3 cameraPos = cameraTransform->GetWorldPosition();
    const glm::vec3 forward = glm::normalize(cameraTransform->Forward());
    return cameraPos + forward * distance;
}
