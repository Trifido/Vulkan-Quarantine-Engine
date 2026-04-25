#include "Light.h"
#include <QEGameObject.h>
#include <algorithm>
#include <glm/common.hpp>

AttenuationData ATTENUATION_TAB[12] = {
        AttenuationData{
            .distance = 7.0f,
            .Linear = 0.7f,
            .Quadratic = 1.8f
        },
        AttenuationData{
            .distance = 13.0f,
            .Linear = 0.35f,
            .Quadratic = 0.44f
        },
        AttenuationData{
            .distance = 20.0f,
            .Linear = 0.22f,
            .Quadratic = 0.2f
        },
        AttenuationData{
            .distance = 32.0f,
            .Linear = 0.14f,
            .Quadratic = 0.07f
        },
        AttenuationData{
            .distance = 50.0f,
            .Linear = 0.09f,
            .Quadratic = 0.032f
        },
        AttenuationData{
            .distance = 65.0f,
            .Linear = 0.07f,
            .Quadratic = 0.017f
        },
        AttenuationData{
            .distance = 100.0f,
            .Linear = 0.045f,
            .Quadratic = 0.027f
        },
        AttenuationData{
            .distance = 160.0f,
            .Linear = 0.027f,
            .Quadratic = 0.0028f
        },
        AttenuationData{
            .distance = 200.0f,
            .Linear = 0.022f,
            .Quadratic = 0.0019f
        },
        AttenuationData{
            .distance = 325.0f,
            .Linear = 0.014f,
            .Quadratic = 0.0007f
        },
        AttenuationData{
            .distance = 600.0f,
            .Linear = 0.007f,
            .Quadratic = 0.0002f
        },
        AttenuationData{
            .distance = 3250.0f,
            .Linear = 0.0014f,
            .Quadratic = 0.000007f
        }
};

QELight::QELight()
{
    this->Name = "QELight";
    this->deviceModule = DeviceModule::getInstance();
    this->uniform = std::make_shared<LightUniform>();

    this->diffuse = this->specular = glm::vec3(0.0f);
    this->constant = 1.0f;
    this->linear = 0.7f;
    this->quadratic = 1.8f;
    this->cutOff = 0.0f;
    this->outerCutoff = 0.0f;
    this->uniform->radius = 1.0f;
    this->idxShadowMap = 0;
}

void QELight::UpdateUniform()
{
    SanitizeParameters();
    ResolveTransformFromOwner();

    this->uniform->lightType = this->lightType;
    this->uniform->specular = this->specular;
    this->uniform->diffuse = this->diffuse;
    this->uniform->constant  = this->constant;
    this->uniform->linear = this->linear;
    this->uniform->quadratic = this->quadratic;
    this->uniform->radius = this->radius;
    this->uniform->idxShadowMap = this->idxShadowMap;
}

void QELight::SetDistanceEffect(float radiusEffect)
{
    this->radius = std::max(radiusEffect, 0.001f);
    this->constant = 1.0f;

    for (int i = 0; i < NUM_ATTENUATIONS; i++)
    {
        if (this->radius < ATTENUATION_TAB[i].distance)
        {
            this->linear = ATTENUATION_TAB[i].Linear;
            this->quadratic = ATTENUATION_TAB[i].Quadratic;
            return;
        }
    }

    this->linear = ATTENUATION_TAB[NUM_ATTENUATIONS - 1].Linear;
    this->quadratic = ATTENUATION_TAB[NUM_ATTENUATIONS - 1].Quadratic;
}

void QELight::SanitizeParameters()
{
    this->constant = std::max(this->constant, 0.0f);
    this->linear = std::max(this->linear, 0.0f);
    this->quadratic = std::max(this->quadratic, 0.0f);
    this->radius = std::max(this->radius, 0.001f);
    this->diffuse = glm::max(this->diffuse, glm::vec3(0.0f));
    this->specular = glm::max(this->specular, glm::vec3(0.0f));

    switch (this->lightType)
    {
    case LightType::POINT_LIGHT:
        this->cutOff = 0.0f;
        this->outerCutoff = 0.0f;
        break;

    case LightType::SPOT_LIGHT:
    {
        const float innerAngleDeg = glm::clamp(glm::degrees(glm::acos(glm::clamp(this->cutOff, -1.0f, 1.0f))), 0.0f, 89.0f);
        const float outerAngleDeg = glm::clamp(glm::degrees(glm::acos(glm::clamp(this->outerCutoff, -1.0f, 1.0f))), innerAngleDeg, 89.5f);

        this->cutOff = glm::cos(glm::radians(innerAngleDeg));
        this->outerCutoff = glm::cos(glm::radians(outerAngleDeg));
        break;
    }

    case LightType::DIRECTIONAL_LIGHT:
    case LightType::SUN_LIGHT:
        this->constant = 1.0f;
        this->linear = 0.0f;
        this->quadratic = 0.0f;
        this->cutOff = 0.0f;
        this->outerCutoff = 0.0f;
        break;

    default:
        break;
    }
}

bool QELight::ResolveTransformFromOwner()
{
    if (this->Owner == nullptr)
    {
        return this->transform != nullptr;
    }

    if (auto ownerTransform = this->Owner->GetComponent<QETransform>())
    {
        if (this->transform != ownerTransform)
        {
            this->transform = ownerTransform;
        }

        return true;
    }

    return this->transform != nullptr;
}

void QELight::QEInit()
{
    ResolveTransformFromOwner();
}

void QELight::QEStart()
{
}
