#pragma once
#ifndef PHYSICS_MODULE_H
#define PHYSICS_MODULE_H

#include <QESingleton.h>

#include <JoltDebugRenderer.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>

class PhysicsModule : public QESingleton<PhysicsModule>
{
private:
    friend class QESingleton<PhysicsModule>;

    std::unique_ptr<JPH::TempAllocatorImpl>   m_temp;
    std::unique_ptr<JPH::JobSystemThreadPool> m_jobs;
    JPH::PhysicsSystem m_system;
    JPH::Vec3 m_gravity = JPH::Vec3(0.0f, -10.0f, 0.0f);
    std::vector<JPH::BodyID> m_bodies;

public:
    JoltDebugRenderer* DebugDrawer = nullptr;

private:
    void ClearAllBodies();
    void UpdateDebugDrawer();

public:
    PhysicsModule();

    inline JPH::PhysicsSystem& World() { return m_system; }
    inline const JPH::PhysicsSystem& World()  const { return m_system; }
    inline JPH::BodyInterface& Bodies() { return m_system.GetBodyInterface(); }
    inline const JPH::BodyInterface& Bodies() const { return m_system.GetBodyInterface(); }

    float GetGravity() const { return m_gravity.GetY(); }
    void SetGravity(float gravity);

    void  ComputePhysics(float fixedDt);

    JPH::BodyID AddRigidBody(const JPH::BodyCreationSettings& settings, JPH::EActivation act = JPH::EActivation::Activate);
    void RemoveRigidBody(JPH::BodyID id);

    // Debug Renderer
    void InitializeDebugResources();
    void CleanupDebugDrawer();

    ~PhysicsModule();
};

#endif

