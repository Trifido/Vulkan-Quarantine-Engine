#include <Collider.h>
#include "SphereCollider.h"

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
    this->Radius = value;

    if (this->colShape != nullptr)
        delete this->colShape;
    this->colShape = new btSphereShape(this->Radius);
}
