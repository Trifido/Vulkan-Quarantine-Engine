#include "Collider.h"

using namespace JPH;

QECollider::QECollider()
{
    colShape = nullptr;
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
    if (!colShape) return;

    RotatedTranslatedShapeSettings s(
        Vec3(displacement.x, displacement.y, displacement.z),
        Quat::sIdentity(),
        colShape
    );

    if (auto res = s.Create(); res.IsValid())
        colShape = res.Get();
    else {
        // opcional: loggear res.GetError()
    }
}
