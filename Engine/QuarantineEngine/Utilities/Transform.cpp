#include "Transform.h"

Transform::Transform()
{
    model = glm::mat4(1.0f);
    view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    proj = glm::mat4(1.0f);
    ubo = std::make_unique<UniformBufferObject>();
}

UniformBufferObject Transform::getMVP()
{
    return *ubo;
}

void Transform::updateMVP(float time, float ratio)
{
    model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    proj = glm::perspective(glm::radians(45.0f), ratio/*extent.width / (float)extent.height*/, 0.1f, 10.0f);
    proj[1][1] *= -1;

    ubo->mvp = proj * view * model;
}
