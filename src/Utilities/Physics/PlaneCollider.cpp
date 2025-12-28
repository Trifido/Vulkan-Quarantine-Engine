#include <PlaneCollider.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

using namespace JPH;

PlaneCollider::PlaneCollider()
{
}

PlaneCollider::PlaneCollider(const float& newSize, const glm::vec3& newOrientation)
{
    this->SetPlane(this->Size, this->Orientation);
}

void PlaneCollider::QEStart()
{
    QECollider::QEStart();

    this->SetPlane(this->Size, this->Orientation);
}

const float PlaneCollider::GetSize()
{
    return this->Size;
}

const glm::vec3 PlaneCollider::GetOrientation()
{
    return this->Orientation;
}

void PlaneCollider::SetPlane(const float& newSize, const glm::vec3& newOrientation)
{
    Size = newSize;
    Orientation = newOrientation;

    // Crea un "suelo" plano usando una caja enorme muy delgada
    BoxShapeSettings settings(
        Vec3(1000.0f, Size, 1000.0f)
    );
    settings.mConvexRadius = 0.0f;

    auto res = settings.Create();
    if (res.IsValid())
        colShape = res.Get();
    else {
        colShape = nullptr;
    }
}
