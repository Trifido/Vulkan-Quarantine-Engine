#pragma once

#ifndef CAMERA_DTO_H
#define CAMERA_DTO_H

#include <glm/glm.hpp>

struct CameraDto
{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float nearPlane;
    float farPlane;
    float fov;

    CameraDto()
        : position{ -10.0f, 5.0f, 0.0f },
        front{ 1.0f, 0.0f, 0.0f },
        up{ 0.0f, 1.0f, 0.0f },
        nearPlane(0.1f),
        farPlane(500.0f),
        fov(45.0f)
    {
    }

    CameraDto(glm::vec3 position, glm::vec3 front, glm::vec3 up, float nearPlane, float farPlane, float fov)
    {
        this->position = position;
        this->front = front;
        this->up = up;
        this->nearPlane = nearPlane;
        this->farPlane = farPlane;
        this->fov = fov;
    }
};

#endif // !CAMERA_DTO_H
