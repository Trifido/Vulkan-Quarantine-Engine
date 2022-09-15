#include "Transform.h"

Transform::Transform()
{
    model = glm::mat4(1.0f);
    view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    proj = glm::mat4(1.0f);
    ubo = UniformBufferObject();
    ubo.mvp = glm::mat4(1.0);
}

UniformBufferObject Transform::getMVP()
{
    return ubo;
}

void Transform::updateMVP(glm::mat4& VPMainCamera)
{
    ubo.mvp = VPMainCamera * model;
}
