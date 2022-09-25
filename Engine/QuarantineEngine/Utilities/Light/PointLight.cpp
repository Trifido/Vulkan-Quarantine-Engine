#include "PointLight.h"

void PointLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = glm::vec4(this->transform->Position, 1.0f);
}
