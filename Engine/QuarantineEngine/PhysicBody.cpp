#include "PhysicBody.h"

PhysicBody::PhysicBody()
{
    this->UpdateType(PhysicBodyType::STATIC_BODY);
}

PhysicBody::PhysicBody(const PhysicBodyType& type)
{
    this->UpdateType(type);
}

void PhysicBody::UpdateType(const PhysicBodyType &type)
{
    this->Type = type;

    if (this->Type == PhysicBodyType::RIGID_BODY)
    {

    }
    else
    {

    }
}
