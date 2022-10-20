#pragma once
#ifndef PHYSICS_MODULE_H
#define PHYSICS_MODULE_H

#include "btBulletDynamicsCommon.h"
class PhysicsModule
{
private:
    static PhysicsModule* instance;

    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

public:
    btAlignedObjectArray<btCollisionShape*> collisionShapes;

public:
    static PhysicsModule* getInstance();
    PhysicsModule();

    void AddRigidBody(btRigidBody* body);
    void ComputePhysics(float deltaTime);

    ~PhysicsModule();
};

#endif

