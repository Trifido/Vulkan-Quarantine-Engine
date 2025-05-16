#pragma once
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "GameComponent.h"
#include <memory>
#include <glm/gtc/quaternion.hpp>
#include <UBO.h>

class Transform : public GameComponent
{
private:
    std::vector<std::shared_ptr<Transform>> childTransforms;
    glm::mat4 parentModel;
    glm::mat4 model;
    glm::mat4 localModel;
    glm::mat4 scale_mat;
    glm::mat4 rot_mat;
    glm::mat4 trans_mat;

public:
    glm::vec3 Position;
    glm::vec3 Direction;
    glm::quat Orientation;
    glm::vec3 Rotation;
    glm::vec3 RadiansRotation;
    glm::vec3 Scale;
    glm::vec3 UpVector;
    glm::vec3 ForwardVector;

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
};

#endif
