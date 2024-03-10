#include "Light.h"

Light::Light()
{
    this->transform = std::make_unique<Transform>();
    this->uniform = std::make_shared<LightUniform>();

    this->diffuse = this->specular = glm::vec3(0.0f);
    this->constant = 1.0f;
    this->linear = 0.7f;
    this->quadratic = 1.8f;
    this->cutOff = 0.0f;
    this->outerCutoff = 0.0f;
    this->uniform->radius = 1.0f;
    this->idxShadowMap = 0;

    this->UpdateUniform();
}

void Light::UpdateUniform()
{
    this->uniform->lightType = this->lightType;
    this->uniform->specular = this->specular;
    this->uniform->diffuse = this->diffuse;
    this->uniform->constant  = this->constant;
    this->uniform->linear = this->linear;
    this->uniform->quadratic = this->quadratic;
    this->uniform->radius = this->radius;
    this->uniform->idxShadowMap = this->idxShadowMap;
}
