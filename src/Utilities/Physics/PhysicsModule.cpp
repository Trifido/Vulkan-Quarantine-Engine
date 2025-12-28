#include "PhysicsModule.h"
#include <iostream>
#include <algorithm>
#include <QECharacterController.h>

using namespace JPH;

namespace
{
    enum BroadPhaseLayers : uint8 { BPL_STATIC, BPL_MOVING, BPL_NUM };

    class BPLayerInterface final : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterface() {
            for (int i = 0; i < 32; ++i) mMap[i] = JPH::BroadPhaseLayer(BPL_MOVING);

            // Mapeos explícitos
            mMap[Layers::SCENE_STATIC] = JPH::BroadPhaseLayer(BPL_STATIC);
            mMap[Layers::TRIGGER_STATIC] = JPH::BroadPhaseLayer(BPL_STATIC);

            mMap[Layers::SCENE_MOVING] = JPH::BroadPhaseLayer(BPL_MOVING);
            mMap[Layers::PLAYER] = JPH::BroadPhaseLayer(BPL_MOVING);
            mMap[Layers::ENEMY] = JPH::BroadPhaseLayer(BPL_MOVING);
            mMap[Layers::TRIGGER_MOVING] = JPH::BroadPhaseLayer(BPL_MOVING);
        }

        uint GetNumBroadPhaseLayers() const override
        {
            return BPL_NUM;
        }

        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            return BroadPhaseLayer(mMap[inLayer]);
        }

        const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            if (inLayer == BroadPhaseLayer(BPL_STATIC)) return "STATIC";
            if (inLayer == BroadPhaseLayer(BPL_MOVING)) return "MOVING";
            return "UNKNOWN";
        }

    private:
        JPH::BroadPhaseLayer mMap[32]{};
    };

    class ObjectVsBroadPhaseFilter final : public ObjectVsBroadPhaseLayerFilter {
    public:
        bool ShouldCollide(ObjectLayer inLayer, BroadPhaseLayer inBPL) const override {
            // Static sólo prueba contra MOVING
            if (inBPL == JPH::BroadPhaseLayer(BPL_STATIC))
                return (
                    inLayer == Layers::PLAYER ||
                    inLayer == Layers::ENEMY ||
                    inLayer == Layers::SCENE_MOVING ||
                    inLayer == Layers::TRIGGER_MOVING
                    );

            // MOVING contra todo (estático + móviles)
            return true;
        }
    };

    class ObjectLayerPairFilterEx final : public ObjectLayerPairFilter {
    public:
        bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override
        {
            auto isScene = [](ObjectLayer l) { return l == Layers::SCENE_STATIC || l == Layers::SCENE_MOVING; };
            auto isTrigger = [](ObjectLayer l) { return l == Layers::TRIGGER_STATIC || l == Layers::TRIGGER_MOVING; };

            // Triggers: siempre dejan pasar para detectar
            if (isTrigger(a) || isTrigger(b)) return true;

            // Escena con escena (STATIC <-> MOVING, MOVING <-> MOVING, etc.)
            if (isScene(a) && isScene(b))
                return true;

            // PLAYER con SCENE
            if ((a == Layers::PLAYER && isScene(b)) || (b == Layers::PLAYER && isScene(a))) return true;

            // PLAYER con ENEMY
            if ((a == Layers::PLAYER && b == Layers::ENEMY) || (b == Layers::PLAYER && a == Layers::ENEMY)) return true;

            // SCENE con ENEMY
            if ((isScene(a) && b == Layers::ENEMY) || (isScene(b) && a == Layers::ENEMY)) return true;

            return false;
        }
    };
}

PhysicsModule::PhysicsModule()
{
    // Jolt: disponer de allocator y fábrica
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    // Allocator temporal (10MB) + jobs (N hilos)
    m_temp = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);
    m_jobs = std::make_unique<JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        std::max(1u, std::thread::hardware_concurrency())
    );

    // Capas y filtros
    m_broadphaseLayers = std::make_unique<BPLayerInterface>();
    m_objectVsBPLFilter = std::make_unique<ObjectVsBroadPhaseFilter>();
    m_pairFilter = std::make_unique<ObjectLayerPairFilterEx>();

    // Init del PhysicsSystem
    constexpr uint32 max_bodies = 8192;
    constexpr uint32 num_body_mutexes = 0;    // auto
    constexpr uint32 max_body_pairs = 8192;
    constexpr uint32 max_contact_constraints = 8192;

    m_system.Init(
        max_bodies,
        num_body_mutexes,
        max_body_pairs,
        max_contact_constraints,
        *m_broadphaseLayers,
        *m_objectVsBPLFilter,
        *m_pairFilter
    );

    // Gravedad (estándar del mundo, distinta de la que uses para CharacterVirtual)
    m_system.SetGravity(m_gravity);

    // Crear renderer de debug para Jolt
    this->DebugDrawer = new JoltDebugRenderer();
}

PhysicsModule::~PhysicsModule()
{
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}

JPH::BodyID PhysicsModule::AddRigidBody(const BodyCreationSettings& settings, EActivation act)
{
    BodyID id = Bodies().CreateAndAddBody(settings, act);
    m_bodies.push_back(id);
    return id;
}

void PhysicsModule::RemoveRigidBody(BodyID id)
{
    if (id.IsInvalid()) return;

    auto& bi = Bodies();
    bi.RemoveBody(id);
    bi.DestroyBody(id);

    auto it = std::find(m_bodies.begin(), m_bodies.end(), id);
    if (it != m_bodies.end()) m_bodies.erase(it);
}

void PhysicsModule::RegisterCharacter(QECharacterController* cc)
{
    if (!cc) return;
    m_characters.push_back(cc);
}

void PhysicsModule::UnregisterCharacter(QECharacterController* cc)
{
    auto it = std::find(m_characters.begin(), m_characters.end(), cc);
    if (it != m_characters.end())
        m_characters.erase(it);
}

void PhysicsModule::ComputePhysics(float fixedDt)
{
    m_system.Update(fixedDt, /*collisionSteps*/1, m_temp.get(), m_jobs.get());
}

void PhysicsModule::SetGravity(float gravityY)
{
    m_gravity = Vec3(0.0f, gravityY, 0.0f);
    m_system.SetGravity(m_gravity);
}

void PhysicsModule::UpdateDebugPhysicsDrawer()
{
    auto debugSystem = QEDebugSystem::getInstance();
    if (!debugSystem->IsEnabled())
        return;

    JPH::BodyManager::DrawSettings draw;
    draw.mDrawWorldTransform = true;

    m_system.DrawBodies(draw, DebugDrawer);

    for (QECharacterController* cc : m_characters)
    {
        if (cc)
        {
            cc->DebugDraw(*DebugDrawer);
        }
    }
}
