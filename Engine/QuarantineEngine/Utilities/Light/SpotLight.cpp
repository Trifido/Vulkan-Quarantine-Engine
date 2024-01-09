#include "SpotLight.h"

SpotLight::SpotLight() : Light()
{
    this->lightType = LightType::SPOT_LIGHT;

    this->spotCutOff = glm::cos(glm::radians(12.5f));
    this->spotExponent = glm::cos(glm::radians(17.5f));
}

void SpotLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->spotCutOff = this->spotCutOff;
    this->uniform->spotExponent = this->spotExponent;
    this->uniform->spotDirection = this->transform->Rotation;
}
