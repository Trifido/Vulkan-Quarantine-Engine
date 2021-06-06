#pragma once
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "GameComponent.h"
#include <memory>

struct UniformBufferObject {
    //glm::mat4 model;
    //glm::mat4 view;
    //glm::mat4 proj;
    glm::mat4 mvp;
};

class Transform : public GameComponent
{
private:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
public:
    std::unique_ptr<UniformBufferObject> ubo;

public:
    Transform();
    UniformBufferObject getMVP();
    void updateMVP(float time, float ratio);
};

#endif
