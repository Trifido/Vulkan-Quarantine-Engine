#include <Collider.h>
#include <BoxCollider.h>

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
    this->Size = value;

    if (this->colShape != nullptr)
    {
        delete this->colShape;
        this->colShape = nullptr;
    }
    this->colShape = new btBoxShape(btVector3(this->Size.x, this->Size.y, this->Size.z));
    this->colShape->setMargin(this->CollisionMargin);
}
