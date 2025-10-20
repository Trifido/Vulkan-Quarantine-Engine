#pragma once
#ifndef QE_TRANSFORM_H
#define QE_TRANSFORM_H

#include "QEGameComponent.h"
#include <memory>
#include <glm/gtc/quaternion.hpp>
#include <UBO.h>

class QETransform : public QEGameComponent, public std::enable_shared_from_this<QETransform>
{
    REFLECTABLE_COMPONENT(QETransform)
public:
    REFLECT_PROPERTY(glm::vec3, localPosition)
    REFLECT_PROPERTY(glm::quat, localRotation)
    REFLECT_PROPERTY(glm::vec3, localScale)
    REFLECT_PROPERTY(std::string, parentId)

private:
    glm::mat4 localMatrix{ 1.0f };
    glm::mat4 worldMatrix{ 1.0f };
    uint32_t worldVersion = 1;
    bool localDirty{ true };
    bool worldDirty{ true };
    bool queuedForUpdate{ false };
    std::weak_ptr<QETransform> parent;
    std::vector<std::shared_ptr<QETransform>> children;

private:
    void EnsureLocal();
    void EnsureWorld();
    void MarkWorldDirty();

public:
    QETransform();
    std::shared_ptr<QETransform> GetPtr() { return shared_from_this(); }

    // Setters locales
    void SetLocalPosition(const glm::vec3& p);
    void SetLocalRotation(const glm::quat& q);
    void SetLocalEulerDegrees(const glm::vec3& deg);
    void SetLocalScale(const glm::vec3& s);

    // Movimientos
    void TranslateLocal(const glm::vec3& d);            // en eje local
    void TranslateWorld(const glm::vec3& d);            // en mundo
    void RotateLocal(const glm::quat& dq);              // q * localRot
    void RotateWorld(const glm::quat& dq);              // world rot
    void SetFromMatrix(const glm::mat4& m);

    // Parenting
    void SetParent(std::shared_ptr<QETransform> newParent, bool keepWorld = true);
    std::shared_ptr<QETransform> GetParent() const { return parent.lock(); }
    std::vector<std::shared_ptr<QETransform>> GetChildren() const { return children; }
    void AddChild(const std::shared_ptr<QETransform>& child);

    // Getters matrices
    const glm::mat4& GetLocalMatrix();
    const glm::mat4& GetWorldMatrix();
    uint32_t GetWorldVersion() const { return worldVersion; }

    // Getters derivados (mundo)
    glm::vec3 GetWorldPosition();
    glm::quat GetWorldRotation();
    glm::vec3 GetWorldScale();

    // Ejes (mundo): RH con forward = -Z
    glm::vec3 Forward();
    glm::vec3 Right();
    glm::vec3 Up();

    // Debug
    void Debug_PrintModel() const;

    void QEStart() override {}
    void QEInit() override {}
    void QEUpdate() override;
    void QEDestroy() override {}
};

#endif
