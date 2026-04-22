#pragma once
#ifndef SUN_LIGHT_H
#define SUN_LIGHT_H

#include "DirectionalLight.h"

class QESunLight : public QEDirectionalLight
{
    REFLECTABLE_DERIVED_COMPONENT(QESunLight, QELight)

private:
    glm::vec3 SunEulerDegrees = glm::vec3(-45.0f, 45.0f, 0.0f);

public:
    SunUniform uniformData;
    std::shared_ptr<UniformBufferObject> sunUBO = nullptr;

    REFLECT_PROPERTY(float, baseIntensity)

public:
    QESunLight();
    void UpdateSun();
    void CleanupSunResources();
    void SetSunEulerDegrees(const glm::vec3& eulerDeg);
    glm::vec3 GetSunEulerDegrees() { return this->SunEulerDegrees; }
};

#endif // !SUN_LIGHT_H
