#include <PhysicsBody.h>
#include <PhysicsModule.h>
#include <QEGameObject.h>

#include <jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <BoxCollider.h>
#include <SphereCollider.h>
#include <CapsuleCollider.h>
#include <PlaneCollider.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

using namespace JPH;

PhysicsBody::PhysicsBody()
{
    this->UpdateType(PhysicBodyType::STATIC_BODY);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
}

PhysicsBody::PhysicsBody(const PhysicBodyType& type)
{
    this->UpdateType(type);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
}

void PhysicsBody::UpdateType(const PhysicBodyType &type)
{
    this->Type = type;
    this->Mass = (this->Type == PhysicBodyType::RIGID_BODY) ? 1.0f : 0.0f;
    this->UpdateInertia(glm::vec3(0.0f));
}

// Helpers de comparación (versión glm)
static inline bool closePos(const glm::vec3& a, const glm::vec3& b, float eps)
{
    glm::vec3 d = a - b;
    return glm::dot(d, d) <= eps * eps;
}

static inline bool closeRot(const glm::quat& a, glm::quat b, float angleEpsRad)
{
    if (glm::dot(a, b) < 0.0f) b = glm::quat(-b.w, -b.x, -b.y, -b.z);
    glm::quat d = glm::inverse(a) * b;
    // angle from quaternion (approx)
    float angle = 2.0f * acosf(glm::clamp(d.w, -1.0f, 1.0f));
    return fabsf(angle) <= angleEpsRad;
}


void PhysicsBody::UpdateInertia(const glm::vec3 &localInertia)
{
    this->Inertia = localInertia;
}

void PhysicsBody::UpdateTransform()
{
    if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->copyTransformtoGLM();
    }
}

JPH::ObjectLayer PhysicsBody::ResolveObjectLayer(const PhysicBodyType type, const CollisionFlag group)
{
    const bool moving = (type != PhysicBodyType::STATIC_BODY);

    if (group == CollisionFlag::COL_SCENE)
        return moving ? Layers::SCENE_MOVING : Layers::SCENE_STATIC;

    if (group == CollisionFlag::COL_PLAYER)
        return Layers::PLAYER;

    if (group == CollisionFlag::COL_ENEMY)
        return Layers::ENEMY;

    if (group == CollisionFlag::COL_TRIGGER)
        return moving ? Layers::TRIGGER_MOVING : Layers::TRIGGER_STATIC;

    // Fallback
    return moving ? Layers::SCENE_MOVING : Layers::SCENE_STATIC;
}

void PhysicsBody::Initialize()
{
    // Requisitos: collider + transform válidos
    if (!collider || !collider->colShape || !transform) return;

    RebuildScaledShapeFromCollider();

    if (!collider->colShape) return;

    // Pose inicial desde tu QETransform
    glm::vec3 pos = transform->GetWorldPosition();
    glm::quat rot = transform->GetWorldRotation();

    Vec3 jpos = toJPH(pos);
    Quat jrot = toJPH(rot);

    // MotionType mapeado desde tu enum
    EMotionType motion = EMotionType::Static;
    switch (this->Type)
    {
        case PhysicBodyType::RIGID_BODY:    motion = EMotionType::Dynamic;   break;
        case PhysicBodyType::KINEMATIC_BODY:motion = EMotionType::Kinematic; break;
        case PhysicBodyType::STATIC_BODY:
        default:                            motion = EMotionType::Static;    break;
    }

    // Layer simple (ajústalo a tu sistema de capas/filtros Jolt)
    // 0: estático, 1: dinámico, 2: kinematic
    JPH::ObjectLayer layer = ResolveObjectLayer(this->Type, this->CollisionGroup);

    BodyCreationSettings s(
        collider->colShape,
        RVec3(jpos.GetX(), jpos.GetY(), jpos.GetZ()),
        jrot,
        motion,
        layer
    );

    if (this->CollisionGroup == CollisionFlag::COL_TRIGGER)
    {
        s.mIsSensor = true;
    }

    if (motion == EMotionType::Dynamic)
    {
        s.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        s.mMassPropertiesOverride.mMass = glm::max(0.0001f, this->Mass);
    }

    // Crea + añade el body
    this->body = PhysicsModule::getInstance()->AddRigidBody(s, JPH::EActivation::Activate);

    // Inicializa prev/curr con la pose inicial
    currPos = pos; currRot = rot;
    prevPos = pos; prevRot = rot;
    hasCurr = true;
}

void PhysicsBody::copyTransformtoGLM()
{
    if (this->body.IsInvalid()) return;

    // Lee la pose interpolada actual (center of mass transform)
    RMat44 m = PhysicsModule::getInstance()->Bodies().GetCenterOfMassTransform(this->body);
    RVec3 p = m.GetTranslation();
    Quat q = m.GetRotation().GetQuaternion();

    glm::vec3 newPos = toGLM(p);
    glm::quat newRot = toGLM(q);

    bool changed = !hasCurr
        || !closePos(currPos, newPos, posEps)
        || !closeRot(currRot, newRot, angEps);

    if (!changed) return;

    transform->TranslateWorld(newPos);
    transform->RotateWorld(newRot);

    currPos = newPos;
    currRot = newRot;
    hasCurr = true;
}

void PhysicsBody::RebuildScaledShapeFromCollider()
{
    using namespace JPH;

    if (!collider || !transform)
        return;

    Vec3 pivotPos = Vec3::sZero();
    Quat pivotRot = Quat::sIdentity();
    Ref<Shape> originalShape = collider->colShape;

    if (originalShape && originalShape->GetSubType() == EShapeSubType::RotatedTranslated)
    {
        const RotatedTranslatedShape* rt =
            static_cast<const RotatedTranslatedShape*>(originalShape.GetPtr());

        pivotPos = rt->GetPosition();
        pivotRot = rt->GetRotation();
    }

    glm::vec3 worldScale = transform->GetWorldScale();
    Ref<Shape> newShape;

    // --- BOX ---
    if (auto box = std::dynamic_pointer_cast<BoxCollider>(collider))
    {
        glm::vec3 halfBase = box->GetSize();
        glm::vec3 halfScaled = halfBase * worldScale;

        BoxShapeSettings settings(Vec3(halfScaled.x, halfScaled.y, halfScaled.z));
        settings.mConvexRadius = collider->CollisionMargin;

        if (auto res = settings.Create(); res.IsValid())
            newShape = res.Get();
    }
    // --- SPHERE ---
    else if (auto sphere = std::dynamic_pointer_cast<SphereCollider>(collider))
    {
        float rBase = sphere->GetRadius();
        float rScale = glm::max(glm::max(glm::abs(worldScale.x), glm::abs(worldScale.y)), glm::abs(worldScale.z));
        float rScaled = rBase * rScale;
        float effectiveRadius = rScaled + collider->CollisionMargin;

        SphereShapeSettings settings(effectiveRadius);
        if (auto res = settings.Create(); res.IsValid())
            newShape = res.Get();
    }
    // --- CAPSULE (eje Y) ---
    else if (auto cap = std::dynamic_pointer_cast<CapsuleCollider>(collider))
    {
        float rBase = cap->GetRadius();
        float hBase = cap->GetHeight();

        float rScale = glm::max(glm::abs(worldScale.x), glm::abs(worldScale.z));
        float hScale = glm::abs(worldScale.y);

        float rScaled = rBase * rScale;
        float hScaled = hBase * hScale;

        float cylinderHeight = glm::max(0.0f, hScaled - 2.0f * rScaled);
        float halfHeight = 0.5f * cylinderHeight;
        float effectiveRadius = rScaled + collider->CollisionMargin;

        CapsuleShapeSettings settings(halfHeight, effectiveRadius);
        if (auto res = settings.Create(); res.IsValid())
            newShape = res.Get();
    }
    // --- PLANE ---
    else if (auto plane = std::dynamic_pointer_cast<PlaneCollider>(collider))
    {
        float thickness = plane->GetSize();

        // Escalamos sólo en XZ; el grosor se mantiene constante
        float scaleXZ = glm::max(glm::abs(worldScale.x), glm::abs(worldScale.z));

        BoxShapeSettings settings(Vec3(1000.0f, thickness, 1000.0f));
        settings.mConvexRadius = 0.0f;

        if (auto res = settings.Create(); res.IsValid())
            newShape = res.Get();
    }

    // Si no hemos podido crear nada, invalidamos la shape
    if (!newShape)
    {
        collider->colShape = nullptr;
        return;
    }

    // 3) Volver a aplicar el pivot, si existía
    bool hasPivot =
        !pivotPos.IsClose(Vec3::sZero()) ||
        !pivotRot.IsClose(Quat::sIdentity());

    if (hasPivot)
    {
        RotatedTranslatedShapeSettings ts(pivotPos, pivotRot, newShape);
        if (auto res2 = ts.Create(); res2.IsValid())
            collider->colShape = res2.Get();
        else
            collider->colShape = nullptr;
    }
    else
    {
        collider->colShape = newShape;
    }
}

void PhysicsBody::QEStart()
{
    QEGameComponent::QEStart();
}

void PhysicsBody::QEInit()
{
    if (this->Owner == nullptr)
        return;

    this->collider = this->Owner->GetComponentInChildren<QECollider>(true);
    this->transform = this->Owner->GetComponent<QETransform>();

    if (this->collider && this->transform)
    {
        this->Initialize();
        QEGameComponent::QEInit();
    }
}

void PhysicsBody::QEUpdate()
{
    if (this->Owner == nullptr) return;

    this->collider = this->Owner->GetComponentInChildren<QECollider>(true);
    this->transform = this->Owner->GetComponent<QETransform>();

    if (this->collider && this->transform && !this->QEInitialized())
    {
        this->Initialize();
        QEGameComponent::QEInit();
    }
}

void PhysicsBody::QEDestroy()
{
    // Quitar y destruir el body si sigue vivo
    if (this->body.IsInvalid())
    {
        QEGameComponent::QEDestroy();
        return;
    }

    auto& bi = PhysicsModule::getInstance()->Bodies();
    bi.RemoveBody(this->body);
    bi.DestroyBody(this->body);
    this->body = BodyID();

    QEGameComponent::QEDestroy();
}

// -------- conversiones --------

inline JPH::Vec3 PhysicsBody::toJPH(const glm::vec3& v)
{
    return JPH::Vec3(v.x, v.y, v.z);
}

inline JPH::Quat PhysicsBody::toJPH(const glm::quat& q)
{
    // glm: w,x,y,z  |  Jolt: x,y,z,w
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

inline glm::vec3 PhysicsBody::toGLM(const JPH::Vec3& v)
{
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

inline glm::quat PhysicsBody::toGLM(const JPH::Quat& q)
{
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}
