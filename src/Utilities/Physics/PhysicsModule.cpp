#include "PhysicsModule.h"
#include <iostream>
#include <algorithm>

using namespace JPH;

namespace
{
    enum BroadPhaseLayers : uint8 { BPL_STATIC, BPL_MOVING, BPL_NUM };

    class BPLayerInterface final : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterface()
        {
            mMap[0] = JPH::BroadPhaseLayer(BPL_STATIC);
            mMap[1] = JPH::BroadPhaseLayer(BPL_MOVING);
            mMap[2] = JPH::BroadPhaseLayer(BPL_MOVING);
        }

        uint GetNumBroadPhaseLayers() const override
        {
            return BPL_NUM;
        }

        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            return BroadPhaseLayer(mMap[inLayer]);
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
        {
            const auto idx = (JPH::BroadPhaseLayer::Type)inLayer;
            switch (idx)
            {
                case (JPH::BroadPhaseLayer::Type)BPL_STATIC:
                    return "STATIC";
                case (JPH::BroadPhaseLayer::Type)BPL_MOVING:
                    return "MOVING";
                default:
                    return "UNKNOWN";
            }
        }
#endif
    private:
        JPH::BroadPhaseLayer mMap[32]{};
    };

    class ObjectVsBroadPhaseFilter final : public ObjectVsBroadPhaseLayerFilter {
    public:
        bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
            if (inLayer1 == 0) return inLayer2 == JPH::BroadPhaseLayer(BPL_MOVING); // estático ve a moving
            if (inLayer1 == 1) return true;                   // dinámico con todo
            if (inLayer1 == 2) return inLayer2 != JPH::BroadPhaseLayer(BPL_STATIC) ? true : true; // ajusta a tus reglas
            return false;
        }
    };

    class ObjectLayerPairFilterEx final : public ObjectLayerPairFilter {
    public:
        bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override {
            // ejemplo: evita character-character si quieres
            if (a == 2 && b == 2) return true; // o false si no quieres colisión entre chars
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
    static BPLayerInterface            broadphase_layers;
    static ObjectVsBroadPhaseFilter    ovb_filter;
    static ObjectLayerPairFilterEx     pair_filter;

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
        broadphase_layers,
        ovb_filter,
        pair_filter
    );

    // Gravedad (estándar del mundo, distinta de la que uses para CharacterVirtual)
    m_system.SetGravity(m_gravity);
}

PhysicsModule::~PhysicsModule()
{
    ClearAllBodies();

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

void PhysicsModule::ComputePhysics(float fixedDt)
{
    m_system.Update(fixedDt, /*collisionSteps*/1, m_temp.get(), m_jobs.get());

    UpdateDebugDrawer();
}

void PhysicsModule::ClearAllBodies()
{
    auto& bi = Bodies();

    for (auto it = m_bodies.rbegin(); it != m_bodies.rend(); ++it)
    {
        BodyID id = *it;

        if (id.IsInvalid()) continue;
        bi.RemoveBody(id);
        bi.DestroyBody(id);
    }

    m_bodies.clear();
}

void PhysicsModule::SetGravity(float gravityY)
{
    m_gravity = Vec3(0.0f, gravityY, 0.0f);
    m_system.SetGravity(m_gravity);
}

void PhysicsModule::UpdateDebugDrawer()
{
    if (!DebugDrawer->IsEnabled())
        return;

    // 1) limpiar líneas previas
    DebugDrawer->clear();

    // 2) pedirle al mundo que “dibuje” hacia nuestro renderer
    JPH::BodyManager::DrawSettings draw;
    // draw.mDrawShape = true;     // por defecto ya dibuja shapes
    // draw.mDrawBodyTransforms = false;
    // draw.mDrawCenterOfMassTransform = false;
    // (activa lo que necesites)

    m_system.DrawBodies(draw, DebugDrawer);

    // 3) subir a GPU
    DebugDrawer->UpdateBuffers();
}


void PhysicsModule::InitializeDebugResources()
{
    // Si ya existe, limpiamos
    if (this->DebugDrawer != nullptr)
    {
        this->DebugDrawer->cleanup();
        delete this->DebugDrawer;
        this->DebugDrawer = nullptr;
    }

    // Crear renderer de debug para Jolt
    this->DebugDrawer = new JoltDebugRenderer();
    this->DebugDrawer->SetEnabled(true);
    this->DebugDrawer->InitializeDebugResources();
}


void PhysicsModule::CleanupDebugDrawer()
{
    if (DebugDrawer != nullptr)
    {
        DebugDrawer->cleanup();
    }
}
