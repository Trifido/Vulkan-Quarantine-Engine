#include "DirectionalLight.h"

DirectionalLight::DirectionalLight() : Light()
{
    this->lightType = LightType::DIRECTIONAL_LIGHT;
}

void DirectionalLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Rotation;
}
