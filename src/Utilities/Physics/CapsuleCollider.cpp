#include "CapsuleCollider.h"
#include <AABBObject.h>
#include <QEGameObject.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

using namespace JPH;

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
        float r = glm::min(boundingBox->Size.x, boundingBox->Size.z) * 0.75f;
        float h = boundingBox->Size.y;

        this->SetSize(r, h);
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
    radius = newRadius;
    height = std::max(0.0f, totalHeight);

    float cylinderHeight = glm::max(0.0f, height - 2.0f * radius);
    float halfHeight = 0.5f * cylinderHeight;
    float effectiveRadius = radius + CollisionMargin;

    CapsuleShapeSettings settings(halfHeight, effectiveRadius);

    if (auto res = settings.Create(); res.IsValid())
        colShape = res.Get();
    else
        colShape = nullptr;
}
