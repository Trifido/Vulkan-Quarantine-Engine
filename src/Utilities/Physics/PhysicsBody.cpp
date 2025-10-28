#include <PhysicsBody.h>
#include <PhysicsModule.h>
#include <QEGameObject.h>

#include <jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

using namespace JPH;

PhysicsBody::PhysicsBody()
{
    this->UpdateType(PhysicBodyType::STATIC_BODY);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
    this->CollisionMask = CollisionFlag::COL_NOTHING;
}

PhysicsBody::PhysicsBody(const PhysicBodyType& type)
{
    this->UpdateType(type);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
    this->CollisionMask = CollisionFlag::COL_NOTHING;
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

void PhysicsBody::Initialize()
{
    // Requisitos: collider + transform válidos
    if (!collider || !collider->colShape || !transform) return;

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
    ObjectLayer layer = (motion == EMotionType::Static) ? 0 : (motion == EMotionType::Dynamic ? 1 : 2);

    BodyCreationSettings s(
        collider->colShape,     // Ref<Shape>
        RVec3(jpos.GetX(), jpos.GetY(), jpos.GetZ()),
        jrot,
        motion,
        layer
    );

    if (motion == EMotionType::Dynamic)
    {
        s.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        s.mMassPropertiesOverride.mMass = glm::max(0.0001f, this->Mass);
    }

    // Crea + añade el body
    JPH::BodyID id = PhysicsModule::getInstance()->AddRigidBody(s, JPH::EActivation::Activate);
    this->body = id;

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

    if (this->collider && this->transform)
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
