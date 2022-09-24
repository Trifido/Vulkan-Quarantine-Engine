#include "Light.h"

Light::Light()
{
    this->transform = std::make_unique<Transform>();
    this->uniform = std::make_shared<LightUniform>();

    this->diffuse = this->specular = glm::vec3(0.0f);

    this->constant = 1.0f;
    this->linear = 0.7f;
    this->quadratic = 1.8f;

    this->UpdateUniform();
}

void Light::UpdateUniform()
{
    this->uniform->specular = this->specular;
    this->uniform->diffuse = this->diffuse;

    this->uniform->constant  = this->constant;
    this->uniform->linear = this->linear;
    this->uniform->quadratic = this->quadratic;
}
