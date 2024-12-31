#pragma once
#ifndef PHYSICS_MODULE_H
#define PHYSICS_MODULE_H

#include <PhysicBody.h>
#include <BoxCollider.h>
#include <SphereCollider.h>
#include <PlaneCollider.h>

#include "btBulletDynamicsCommon.h"
#include <QESingleton.h>

class PhysicsModule : public QESingleton<PhysicsModule>
{
private:
    friend class QESingleton<PhysicsModule>; // Permitir acceso al constructor

    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;

public:
    btDiscreteDynamicsWorld* dynamicsWorld;
    btAlignedObjectArray<btCollisionShape*> collisionShapes;

public:
    PhysicsModule();

    void AddRigidBody(btRigidBody* body);
    void ComputePhysics(float deltaTime);

    ~PhysicsModule();
};

#endif

