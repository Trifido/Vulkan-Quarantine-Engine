#include "SpotLight.h"

SpotLight::SpotLight() : Light()
{
    this->lightType = LightType::SPOT_LIGHT;

    this->cutOff = glm::cos(glm::radians(12.5f));
    this->outerCutoff = glm::cos(glm::radians(17.5f));
}

void SpotLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->cutOff = this->cutOff;
    this->uniform->outerCutoff = this->outerCutoff;
    this->uniform->direction = this->transform->Rotation;
}
