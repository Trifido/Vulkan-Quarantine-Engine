#include "Transform.h"
#include <glm/gtx/quaternion.hpp>

Transform::Transform()
{
    this->model = glm::mat4(1.0f);
    this->ResetTransform();
    this->UpVector = glm::vec3(0.0f, 1.0f, 0.0f);
    this->ForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
}

Transform::Transform(glm::mat4 model) : Transform()
{
    this->model = model;
}

void Transform::SetPosition(const glm::vec3& newPosition)
{
    this->Position = newPosition;
    this->trans_mat = glm::translate(glm::mat4(1.0f), this->Position);
    this->model = this->trans_mat * this->rot_mat * this->scale_mat;
}

void Transform::SetOrientation(const glm::vec3& newRotation)
{
    this->Rotation = newRotation;

    this->RadiansRotation = glm::vec3(glm::radians(newRotation));
    this->Orientation = glm::quat(this->RadiansRotation);
    this->rot_mat = glm::toMat4(this->Orientation);
    this->model = this->trans_mat * this->rot_mat * this->scale_mat;

    glm::mat4 rotation_x = glm::rotate(glm::mat4(1.0f), this->RadiansRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0f), this->RadiansRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotation_z = glm::rotate(glm::mat4(1.0f), this->RadiansRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 combined_rotation = rotation_z * rotation_y * rotation_x;

    this->ForwardVector = combined_rotation * glm::vec4(this->ForwardVector, 0.0f);
    this->UpVector = combined_rotation * glm::vec4(this->UpVector, 0.0f);
}

void Transform::SetScale(const glm::vec3& newScale)
{
    this->Scale = newScale;
    this->scale_mat = glm::scale(glm::mat4(1.0f), this->Scale);
    this->model = this->trans_mat * this->rot_mat * this->scale_mat;
}

void Transform::ResetTransform()
{
    this->Position = glm::vec3(0.0f);
    this->Direction = glm::vec3(0.0f);
    this->Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    this->Rotation = glm::vec3(0.0f);
    this->RadiansRotation = glm::vec3(glm::radians(this->Rotation));
    this->Scale = glm::vec3(1.0f);
    this->model = glm::mat4(1.0f);

    this->trans_mat = glm::mat4(1.0f);
    this->rot_mat = glm::mat4(1.0f);
    this->scale_mat = glm::mat4(1.0f);
}

const glm::mat4& Transform::GetModel()
{
    return model;
}

void Transform::SetModel(const glm::mat4& newModel)
{
    this->model = newModel;
}
