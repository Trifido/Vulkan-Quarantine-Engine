#include "CapsuleCollider.h"

CapsuleCollider::CapsuleCollider() : Collider()
{
    this->SetSize(0.5f, 1.0f);
}

CapsuleCollider::CapsuleCollider(float newRadius, float newHeight)
{
    this->SetSize(newRadius, newHeight);
}

void CapsuleCollider::SetSize(float newRadius, float newHeight)
{
    this->radius = newRadius;
    this->height = newHeight;

    if (this->colShape != nullptr)
        delete this->colShape;

    colShape = new btCapsuleShape(this->radius, this->height);
    colShape->setMargin(CollisionMargin);
    colShape->setLocalScaling(btVector3(1, 1, 1));
    colShape->setUserPointer(this);
}
