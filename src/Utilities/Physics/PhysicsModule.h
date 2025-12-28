#pragma once
#ifndef PHYSICS_MODULE_H
#define PHYSICS_MODULE_H

#include <QESingleton.h>

#include <PhysicsTypes.h>
#include <JoltDebugRenderer.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>

namespace Layers
{
    enum : JPH::ObjectLayer
    {
        SCENE_STATIC = 0,      // BroadPhase STATIC
        SCENE_MOVING,          // ProadPhase MOVING
        PLAYER,                // BroadPhase MOVING
        ENEMY,                 // BroadPhase MOVING
        TRIGGER_STATIC,        // sensor STATIC
        TRIGGER_MOVING,        // sensor MOVING
        NUM_LAYERS
    };
}

class QECharacterController;

class PhysicsModule : public QESingleton<PhysicsModule>
{
private:
    friend class QESingleton<PhysicsModule>;

    std::unique_ptr<JPH::TempAllocatorImpl>   m_temp;
    std::unique_ptr<JPH::JobSystemThreadPool> m_jobs;
    JPH::PhysicsSystem m_system;
    JPH::Vec3 m_gravity = JPH::Vec3(0.0f, -10.0f, 0.0f);
    std::vector<JPH::BodyID> m_bodies;
    std::vector<QECharacterController*> m_characters;

    std::unique_ptr<JPH::BroadPhaseLayerInterface> m_broadphaseLayers;
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_objectVsBPLFilter;
    std::unique_ptr<JPH::ObjectLayerPairFilter> m_pairFilter;
public:
    JoltDebugRenderer* DebugDrawer = nullptr;

public:
    PhysicsModule();

    void UpdateDebugPhysicsDrawer();

    const JPH::BroadPhaseLayerInterface& GetBPLayerInterface()   const { return *m_broadphaseLayers; }
    const JPH::ObjectVsBroadPhaseLayerFilter& GetObjectVsBPLFilter()  const { return *m_objectVsBPLFilter; }
    const JPH::ObjectLayerPairFilter& GetObjectLayerPairFilter() const { return *m_pairFilter; }

    class BodyFilterSelfIgnore : public JPH::BodyFilter
    {
        JPH::BodyID mIgnore;
    public:
        explicit BodyFilterSelfIgnore(JPH::BodyID id) : mIgnore(id) {}
        bool ShouldCollide(const JPH::BodyID& other) const override { return other != mIgnore; }
    };
    JPH::BodyFilter* GetBodyFilterSelfIgnore(JPH::BodyID id) const { return new BodyFilterSelfIgnore(id); }

    inline JPH::PhysicsSystem& World() { return m_system; }
    inline const JPH::PhysicsSystem& World()  const { return m_system; }
    inline JPH::BodyInterface& Bodies() { return m_system.GetBodyInterface(); }
    inline const JPH::BodyInterface& Bodies() const { return m_system.GetBodyInterface(); }

    float GetGravity() const { return m_gravity.GetY(); }
    void SetGravity(float gravity);

    void  ComputePhysics(float fixedDt);

    JPH::BodyID AddRigidBody(const JPH::BodyCreationSettings& settings, JPH::EActivation act = JPH::EActivation::Activate);
    void RemoveRigidBody(JPH::BodyID id);
    void RegisterCharacter(QECharacterController* cc);
    void UnregisterCharacter(QECharacterController* cc);

    ~PhysicsModule();
};

#endif

