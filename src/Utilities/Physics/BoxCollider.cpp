#include <BoxCollider.h>
#include <AABBObject.h>
#include <QEGameObject.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

using namespace JPH;

BoxCollider::BoxCollider()
{
    this->SetSize(glm::vec3(0.5f));
}

BoxCollider::BoxCollider(const glm::vec3& newSize)
{
    this->SetSize(newSize);
}

const glm::vec3 BoxCollider::GetSize()
{
    return this->Size;
}

bool BoxCollider::TryAutoFit()
{
    if (!Owner)
        return false;

    auto boundingBox = Owner->GetComponentInChildren<AABBObject>(true);
    if (!boundingBox)
        return false;

    SetSize(glm::max(boundingBox->Size * 0.5f, glm::vec3(0.001f)));
    SetColliderPivot(boundingBox->Center);
    return true;
}

void BoxCollider::QEInit()
{
    if (QEInitialized())
        return;

    _autoFitted = TryAutoFit();
    QECollider::QEInit();
}

void BoxCollider::QEUpdate()
{
    QECollider::QEUpdate();
}

void BoxCollider::SetSize(const glm::vec3& value)
{
    Size = value;

    BoxShapeSettings settings(Vec3(Size.x, Size.y, Size.z));
    settings.mConvexRadius = CollisionMargin;

    if (auto res = settings.Create(); res.IsValid())
        colShape = res.Get();
    else
        colShape = nullptr;
}

