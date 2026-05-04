#include "SampleGameplayComponent.h"

#include <QEGameObject.h>
#include <QETransform.h>
#include <glm/gtc/quaternion.hpp>

SampleGameplayComponent::SampleGameplayComponent()
{
    RotationSpeed = 35.0f;
}

void SampleGameplayComponent::QEUpdate()
{
    if (Owner == nullptr)
    {
        return;
    }

    auto transform = Owner->GetComponent<QETransform>();
    if (transform == nullptr)
    {
        return;
    }

    transform->RotateLocal(glm::quat(glm::radians(glm::vec3(0.0f, RotationSpeed * 0.016f, 0.0f))));
}
