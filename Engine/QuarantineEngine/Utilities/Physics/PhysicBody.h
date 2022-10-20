#pragma once
#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

enum PhysicBodyType
{
    STATIC_BODY,
    RIGID_BODY
};

class PhysicBody
{
public:
    PhysicBodyType Type;
public:
    PhysicBody();
    PhysicBody(const PhysicBodyType& type);
    void UpdateType(const PhysicBodyType& type);
};

#endif
