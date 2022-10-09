#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
{
    this->transform = std::make_unique<Transform>();
    this->uniform = std::make_shared<LightUniform>();

    this->diffuse = this->specular = glm::vec3(0.0f);

    this->constant = 1.0f;
    this->linear = 0.0f;
    this->quadratic = 0.0f;
}

void DirectionalLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = glm::vec4(this->transform->Rotation, 0.0f);
}
