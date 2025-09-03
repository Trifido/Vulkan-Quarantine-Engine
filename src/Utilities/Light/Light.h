#pragma once
#ifndef QELIGHT_H
#define QELIGHT_H

#include "Transform.h"
#include "DeviceModule.h"
#include <LightType.h>
#include <QEGameComponent.h>

struct AttenuationData
{
    float distance;
    float Linear;
    float Quadratic;
};

const uint32_t NUM_ATTENUATIONS = 12;

class QELight : public QEGameComponent
{
protected:
    REFLECTABLE_DERIVED_COMPONENT(QELight, QEGameComponent)

protected:
    DeviceModule* deviceModule = nullptr;

    REFLECT_PROPERTY(float, constant)
    REFLECT_PROPERTY(float, linear)
    REFLECT_PROPERTY(float, quadratic)
    REFLECT_PROPERTY(float, radius)

public:
    std::shared_ptr<Transform> transform;
    std::shared_ptr<LightUniform> uniform;

    bool ResourcesInitialized = false;

    REFLECT_PROPERTY(std::string, Name)
    REFLECT_PROPERTY(LightType, lightType)
    REFLECT_PROPERTY(uint32_t, idxShadowMap)
    REFLECT_PROPERTY(glm::vec3, diffuse)
    REFLECT_PROPERTY(glm::vec3, specular)
    REFLECT_PROPERTY(float, cutOff)
    REFLECT_PROPERTY(float, outerCutoff)
public:
    QELight();
    virtual void UpdateUniform();
    void SetDistanceEffect(float radiusEffect);
    float GetDistanceEffect() { return this->radius; }

    void QEStart() override;
protected:
    void QEInit() override;
};

#endif
