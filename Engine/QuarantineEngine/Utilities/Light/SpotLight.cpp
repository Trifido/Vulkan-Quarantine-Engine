#include "SpotLight.h"

SpotLight::SpotLight()
{
    this->transform = std::make_unique<Transform>();
    this->uniform = std::make_shared<LightUniform>();

    this->diffuse = this->specular = glm::vec3(0.0f);

    this->constant = 1.0f;
    this->linear = 0.0f;
    this->quadratic = 0.0f;

    this->spotCutOff = glm::cos(glm::radians(12.5f));
    this->spotExponent = glm::cos(glm::radians(17.5f));
}

void SpotLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = glm::vec4(this->transform->Position, 0.0f);
    this->uniform->spotCutOff = this->spotCutOff;
    this->uniform->spotExponent = this->spotExponent;
    this->uniform->spotDirection = this->transform->Rotation;
}
