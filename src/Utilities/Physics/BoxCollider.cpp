#include <BoxCollider.h>
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
