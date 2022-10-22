#include <Collider.h>
#include <PlaneCollider.h>

PlaneCollider::PlaneCollider()
{
    this->SetPlane(1.0f,glm::vec3(1.0f));
}

PlaneCollider::PlaneCollider(const float& newSize, const glm::vec3& newOrientation)
{
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
    this->Size = newSize;
    this->Orientation = newOrientation;

    if (this->colShape != nullptr)
        delete this->colShape;
    this->colShape = new btStaticPlaneShape(btVector3(this->Orientation.x, this->Orientation.y, this->Orientation.z), this->Size);
}
