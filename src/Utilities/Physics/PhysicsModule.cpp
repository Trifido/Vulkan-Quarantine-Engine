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
            // Tabla conceptual (simétrica):
            // - PLAYER colisiona con SCENE y ENEMY, NO con TRIGGER
            // - ENEMY colisiona con SCENE y PLAYER, NO con TRIGGER
            // - SCENE colisiona con PLAYER, ENEMY, NO con TRIGGER
            // - TRIGGER detecta a PLAYER/ENEMY/SCENE pero como sensor (mIsSensor ya evita respuesta)
            auto isScene = [](ObjectLayer l) { return l == Layers::SCENE_STATIC || l == Layers::SCENE_MOVING; };
            auto isTrigger = [](ObjectLayer l) { return l == Layers::TRIGGER_STATIC || l == Layers::TRIGGER_MOVING; };

            // Triggers: dejan pasar para "detectar" (si quieres que NO generen contacto, ya lo evita mIsSensor)
            if (isTrigger(a) || isTrigger(b)) return true;

            // Reglas “máscaras”:
            if ((a == Layers::PLAYER && isScene(b)) || (b == Layers::PLAYER && isScene(a))) return true;
            if ((a == Layers::PLAYER && b == Layers::ENEMY) || (b == Layers::PLAYER && a == Layers::ENEMY)) return true;

            // Escena con Enemigo
            if ((isScene(a) && b == Layers::ENEMY) || (isScene(b) && a == Layers::ENEMY)) return true;

            // Si nada aplica, no colisionan
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

void PhysicsModule::ComputePhysics(float fixedDt)
{
    m_system.Update(fixedDt, /*collisionSteps*/1, m_temp.get(), m_jobs.get());
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
    draw.mDrawWorldTransform = true;
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
