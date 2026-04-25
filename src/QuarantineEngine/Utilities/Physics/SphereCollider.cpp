#include "SphereCollider.h"
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

using namespace JPH;

SphereCollider::SphereCollider()
{
    this->SetRadius(1.0f);
}

SphereCollider::SphereCollider(const float& radius)
{
    this->SetRadius(radius);
}

const float SphereCollider::GetRadius()
{
    return this->Radius;
}

void SphereCollider::SetRadius(const float& value)
{
    Radius = value;

    const float effectiveRadius = Radius + CollisionMargin;

    JPH::SphereShapeSettings settings(effectiveRadius);
    if (auto res = settings.Create(); res.IsValid())
        colShape = res.Get();
    else
        colShape = nullptr;
}
