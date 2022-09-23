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
    glm::mat4 model;
    glm::mat4 scale_mat;
    glm::mat4 rot_mat;
    glm::mat4 trans_mat;

    glm::vec3 Position;
    glm::quat Orientation;
    glm::vec3 Scale;
public:

    TransformUniform ubo;

public:
    Transform();
    TransformUniform getMVP();
    void updateModelUniform();

    void SetPosition(const glm::vec3& newPosition);
    void SetOrientation(const glm::vec3& newRotation);
    void SetScale(const glm::vec3& newScale);
    void ResetTransform();
    const glm::mat4& GetModel();
};

#endif
