#include "PhysicsModule.h"
#include <iostream>
#include <algorithm>
#include <QECharacterController.h>
#include <PhysicsBody.h>

using namespace JPH;

namespace
{
    class QECollisionMaskFilter final : public GroupFilter
    {
    public:
        bool CanCollide(const CollisionGroup& inGroup1, const CollisionGroup& inGroup2) const override
        {
            const uint32_t groupA = inGroup1.GetGroupID();
            const uint32_t groupB = inGroup2.GetGroupID();
            const uint32_t maskA = inGroup1.GetSubGroupID();
            const uint32_t maskB = inGroup2.GetSubGroupID();

            if (groupA == 0 || groupB == 0)
                return false;

            return (maskA & groupB) != 0 && (maskB & groupA) != 0;
        }
    };

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
            auto isStaticLayer = [](ObjectLayer l)
            {
                return l == Layers::SCENE_STATIC || l == Layers::TRIGGER_STATIC;
            };

            if (isStaticLayer(a) && isStaticLayer(b))
                return false;

            return true;
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
    m_collisionFilter = new QECollisionMaskFilter();

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
    delete DebugDrawer;
    DebugDrawer = nullptr;

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
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

void PhysicsModule::RegisterBodyComponent(PhysicsBody* bodyComponent)
{
    if (!bodyComponent)
        return;

    if (std::find(m_bodyComponents.begin(), m_bodyComponents.end(), bodyComponent) == m_bodyComponents.end())
        m_bodyComponents.push_back(bodyComponent);
}

void PhysicsModule::UnregisterBodyComponent(PhysicsBody* bodyComponent)
{
    auto it = std::find(m_bodyComponents.begin(), m_bodyComponents.end(), bodyComponent);
    if (it != m_bodyComponents.end())
        m_bodyComponents.erase(it);
}

void PhysicsModule::SyncEditorBodies()
{
    for (PhysicsBody* bodyComponent : m_bodyComponents)
    {
        if (bodyComponent)
            bodyComponent->RefreshEditorState();
    }
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

    DebugDrawer->NextFrame();
}

