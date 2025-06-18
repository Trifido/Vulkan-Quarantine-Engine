#pragma once
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "QEGameComponent.h"
#include <memory>
#include <glm/gtc/quaternion.hpp>
#include <UBO.h>

class Transform : public QEGameComponent
{
    REFLECTABLE_COMPONENT(Transform)
private:
    std::vector<std::shared_ptr<Transform>> childTransforms;
    REFLECT_PROPERTY(std::vector<std::string>, childIDs)
    REFLECT_PROPERTY(glm::mat4, parentModel)
    REFLECT_PROPERTY(glm::mat4, model)
    REFLECT_PROPERTY(glm::mat4, localModel)
    REFLECT_PROPERTY(glm::mat4, scale_mat)
    REFLECT_PROPERTY(glm::mat4, rot_mat)
    REFLECT_PROPERTY(glm::mat4, trans_mat)

public:
    REFLECT_PROPERTY(glm::vec3, Position)
    REFLECT_PROPERTY(glm::vec3, Direction)
    REFLECT_PROPERTY(glm::quat, Orientation)
    REFLECT_PROPERTY(glm::vec3, Rotation)
    REFLECT_PROPERTY(glm::vec3, RadiansRotation)
    REFLECT_PROPERTY(glm::vec3, Scale)
    REFLECT_PROPERTY(glm::vec3, UpVector)
    REFLECT_PROPERTY(glm::vec3, ForwardVector)

private:
    void ReceiveNewParentModel(glm::mat4 parentModel);
    void SendNewParentModel();

public:
    Transform();
    Transform(glm::mat4 model);

    void SetPosition(const glm::vec3& newPosition);
    void SetOrientation(const glm::vec3& newRotation);
    void SetScale(const glm::vec3& newScale);
    void ResetTransform();
    const glm::mat4& GetModel();
    void SetModel(const glm::mat4& newModel);
    void AddChild(std::shared_ptr<Transform> child);

    void Debug_PrintModel() const;

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif
