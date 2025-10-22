#include "CapsuleCollider.h"
#include <AABBObject.h>
#include <QEGameObject.h>

void CapsuleCollider::QEStart()
{
    QECollider::QEStart();
}

void CapsuleCollider::QEInit()
{
    if (QEInitialized()) return;

    auto boundingBox = this->Owner->GetComponentInChildren<AABBObject>(true);

    if (boundingBox != nullptr)
    {
        this->compound = new btCompoundShape();
        this->SetSize(glm::min(boundingBox->Size.x, boundingBox->Size.z) * 0.75f, boundingBox->Size.y);
        this->SetColliderPivot(boundingBox->Center);
    }

    QECollider::QEInit();
}

CapsuleCollider::CapsuleCollider() : QECollider()
{
}

CapsuleCollider::CapsuleCollider(float newRadius, float newHeight)
{
    this->radius = newRadius;
    this->height = newHeight;
}

void CapsuleCollider::SetSize(float newRadius, float totalHeight)
{
    this->radius = newRadius;
    this->height = std::max(0.0f, totalHeight);

    if (this->colShape != nullptr)
        delete this->colShape;

    colShape = new btCapsuleShape(this->radius, this->height - (2.0f * this->radius));
    colShape->setMargin(CollisionMargin);
    colShape->setLocalScaling(btVector3(1, 1, 1));
    colShape->setUserPointer(this);
}
