#include "Collider.h"

QECollider::QECollider()
{
    colShape = nullptr;
    compound = nullptr;
    CollisionMargin = 0.04f;
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

void QECollider::SetColliderPivot(glm::vec3 displacement)
{
    btTransform localTransform;
    localTransform.setIdentity();
    localTransform.setOrigin(btVector3(displacement.x, displacement.y, displacement.z));

    auto numchildShape = compound->getNumChildShapes();
    if (numchildShape == 0)
    {
        compound->addChildShape(localTransform, colShape);
    }
    else
    {
        compound->removeChildShape(0);
        compound->addChildShape(localTransform, colShape);
    }
}
