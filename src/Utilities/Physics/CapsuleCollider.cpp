#include "CapsuleCollider.h"
#include <AABBObject.h>
#include <QEGameObject.h>

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

using namespace JPH;

void CapsuleCollider::QEStart()
{
    QECollider::QEStart();
}

bool CapsuleCollider::TryAutoFit()
{
    auto boundingBox = Owner->GetComponentInChildren<AABBObject>(true);
    if (!boundingBox) return false;

    float r = glm::min(boundingBox->Size.x, boundingBox->Size.z) * 0.75f;
    float h = boundingBox->Size.y;

    SetSize(r, h);
    SetColliderPivot(boundingBox->Center);
    return true;
}

void CapsuleCollider::QEInit()
{
    if (QEInitialized()) return;

    _autoFitted = TryAutoFit();
    QECollider::QEInit();
}

void CapsuleCollider::QEUpdate()
{
    if (!_autoFitted)
        _autoFitted = TryAutoFit();

    QECollider::QEUpdate();
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
