#pragma once

#include <memory>
#include <glm/glm.hpp>

class QEGameObject;
class QECamera;
class QETransform;

class QEGizmoController
{
public:
    enum class Operation
    {
        None = 0,
        Translate,
        Rotate,
        Scale
    };

    enum class Space
    {
        Local = 0,
        World
    };

public:
    QEGizmoController();

    void SetOperation(Operation operation);
    Operation GetOperation() const;

    void SetSpace(Space space);
    Space GetSpace() const;

    void SetSnapEnabled(bool value);
    bool IsSnapEnabled() const;

    void SetTranslationSnap(const glm::vec3& value);
    void SetRotationSnap(const float& degrees);
    void SetScaleSnap(const glm::vec3& value);

    bool Draw(
        std::shared_ptr<QEGameObject> selectedObject,
        std::shared_ptr<QECamera> camera,
        const glm::vec2& viewportPosition,
        const glm::vec2& viewportSize);

    bool IsUsing() const;
    bool IsOver() const;

private:
    bool ManipulateTransform(
        std::shared_ptr<QEGameObject> selectedObject,
        std::shared_ptr<QETransform> transform,
        std::shared_ptr<QECamera> camera,
        const glm::vec2& viewportPosition,
        const glm::vec2& viewportSize);

private:
    Operation _operation;
    Space _space;

    bool _snapEnabled;
    glm::vec3 _translationSnap;
    float _rotationSnapDegrees;
    glm::vec3 _scaleSnap;
};
