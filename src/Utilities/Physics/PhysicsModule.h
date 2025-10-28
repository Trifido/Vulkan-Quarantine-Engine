#pragma once
#ifndef PHYSICS_MODULE_H
#define PHYSICS_MODULE_H

#include <PhysicsBody.h>
#include <BoxCollider.h>
#include <SphereCollider.h>
#include <PlaneCollider.h>
#include <BulletDebugDrawer.h>

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
    float gravity = -10.0f;

public:
    BulletDebugDrawer* debugDrawer;
    btDiscreteDynamicsWorld* dynamicsWorld;
    btAlignedObjectArray<btCollisionShape*> collisionShapes;

private:
    void UpdateDebugDrawer();

public:
    PhysicsModule();

    float GetGravity() const { return gravity; }
    void SetGravity(float gravity);
    void AddRigidBody(btRigidBody* body);
    void ComputePhysics(float fixedDt);
    void CleanupDebugDrawer();
    void InitializeDebugResources();

    ~PhysicsModule();
};

#endif

