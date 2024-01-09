#include "PointLight.h"

PointLight::PointLight() : Light()
{
    this->lightType = LightType::POINT_LIGHT;
}

void PointLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->radius = 1.0f;
    this->uniform->lightType = 0;
}
