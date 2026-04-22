#include <PlaneCollider.h>
#include <AABBObject.h>
#include <QEGameObject.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

using namespace JPH;

PlaneCollider::PlaneCollider()
{
    this->Size = 0.001f;
    this->Extents = glm::vec2(0.5f, 0.5f);
    this->Orientation = glm::vec3(0.0f, 1.0f, 0.0f);
    this->SetPlane(this->Size, this->Orientation);
}

PlaneCollider::PlaneCollider(const float& newSize, const glm::vec3& newOrientation)
{
    this->SetPlane(newSize, newOrientation);
}

void PlaneCollider::QEStart()
{
    QECollider::QEStart();

    this->SetPlane(this->Size, this->Orientation);
}

void PlaneCollider::QEInit()
{
    if (QEInitialized())
        return;

    _autoFitted = TryAutoFit();
    QECollider::QEInit();
}

const float PlaneCollider::GetSize()
{
    return this->Size;
}

const glm::vec2 PlaneCollider::GetExtents()
{
    return this->Extents;
}

const glm::vec3 PlaneCollider::GetOrientation()
{
    return this->Orientation;
}

bool PlaneCollider::TryAutoFit()
{
    if (!Owner)
        return false;

    auto boundingBox = Owner->GetComponentInChildren<AABBObject>(true);
    if (!boundingBox)
        return false;

    Extents = glm::max(glm::vec2(boundingBox->Size.x, boundingBox->Size.z) * 0.5f, glm::vec2(0.001f));
    Size = glm::max(Size, 0.001f);
    SetPlane(Size, Orientation);
    SetColliderPivot(boundingBox->Center);
    return true;
}

void PlaneCollider::SetPlane(const float& newSize, const glm::vec3& newOrientation)
{
    Size = glm::max(newSize, 0.001f);
    Orientation = newOrientation;
    Extents = glm::max(Extents, glm::vec2(0.001f));

    // Representamos el plano como una caja finita y delgada para que el debug draw coincida con el editor.
    BoxShapeSettings settings(
        Vec3(Extents.x, Size, Extents.y)
    );
    settings.mConvexRadius = 0.0f;

    auto res = settings.Create();
    if (res.IsValid())
        colShape = res.Get();
    else {
        colShape = nullptr;
    }
}
