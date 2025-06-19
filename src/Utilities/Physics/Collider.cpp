#include "Collider.h"

QECollider::QECollider()
{
    CollisionMargin = 0.04f;
    LocalDisplacement = glm::vec3(0.0f);
}

void QECollider::QEStart()
{
    QEGameComponent::QEStart();
}

void QECollider::QEInit()
{
    QEGameComponent::QEInit();
}

void QECollider::QEUpdate()
{
}

void QECollider::QEDestroy()
{
    QEGameComponent::QEDestroy();
}
