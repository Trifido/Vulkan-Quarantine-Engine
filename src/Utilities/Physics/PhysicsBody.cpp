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
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/MotionProperties.h>

using namespace JPH;

PhysicsBody::PhysicsBody()
{
    this->UpdateType(PhysicBodyType::STATIC_BODY);
    this->CollisionGroup = CollisionFlag::COL_SCENE;
    this->CollisionMask = CollisionFlag::COL_ALL;
    this->Friction = 0.2f;
    this->Restitution = 0.0f;
    this->LinearDamping = 0.05f;
    this->AngularDamping = 0.05f;
}

PhysicsBody::PhysicsBody(const PhysicBodyType& type)
{
    this->UpdateType(type);
    this->CollisionGroup = CollisionFlag::COL_SCENE;
    this->CollisionMask = CollisionFlag::COL_ALL;
    this->Friction = 0.2f;
    this->Restitution = 0.0f;
    this->LinearDamping = 0.05f;
    this->AngularDamping = 0.05f;
}

void PhysicsBody::UpdateType(const PhysicBodyType &type)
{
    this->Type = type;
    this->Mass = (this->Type == PhysicBodyType::RIGID_BODY) ? 1.0f : 0.0f;
    this->UpdateInertia(glm::vec3(0.0f));
}

static inline bool closePos(const glm::vec3& a, const glm::vec3& b, float eps)
{
    glm::vec3 d = a - b;
    return glm::dot(d, d) <= eps * eps;
}

static inline bool closeRot(const glm::quat& a, glm::quat b, float angleEpsRad)
{
    if (glm::dot(a, b) < 0.0f) b = glm::quat(-b.w, -b.x, -b.y, -b.z);
    glm::quat d = glm::inverse(a) * b;
    float angle = 2.0f * acosf(glm::clamp(d.w, -1.0f, 1.0f));
    return fabsf(angle) <= angleEpsRad;
}

static int DetectColliderKind(const std::shared_ptr<QECollider>& collider)
{
    if (!collider)
        return 0;
    if (std::dynamic_pointer_cast<BoxCollider>(collider))
        return 1;
    if (std::dynamic_pointer_cast<SphereCollider>(collider))
        return 2;
    if (std::dynamic_pointer_cast<CapsuleCollider>(collider))
        return 3;
    if (std::dynamic_pointer_cast<PlaneCollider>(collider))
        return 4;
    return 5;
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
    const CollisionFlag effectiveGroup = SanitizeCollisionGroup(group);
    const bool moving = (type != PhysicBodyType::STATIC_BODY);

    if (effectiveGroup == CollisionFlag::COL_SCENE)
        return moving ? Layers::SCENE_MOVING : Layers::SCENE_STATIC;

    if (effectiveGroup == CollisionFlag::COL_PLAYER)
        return Layers::PLAYER;

    if (effectiveGroup == CollisionFlag::COL_ENEMY)
        return Layers::ENEMY;

    if (effectiveGroup == CollisionFlag::COL_TRIGGER)
        return moving ? Layers::TRIGGER_MOVING : Layers::TRIGGER_STATIC;

    // Fallback
    return moving ? Layers::SCENE_MOVING : Layers::SCENE_STATIC;
}

void PhysicsBody::Initialize()
{
    if (_QEInitialized || !collider || !collider->colShape || !transform) return;

    if (!body.IsInvalid())
    {
        return;
    }
    else
    {
        PhysicsModule::getInstance()->RemoveRigidBody(body);
        body = JPH::BodyID();
    }

    RebuildScaledShapeFromCollider();

    if (!collider->colShape) return;

    glm::vec3 pos = transform->GetWorldPosition();
    glm::quat rot = transform->GetWorldRotation();

    Vec3 jpos = toJPH(pos);
    Quat jrot = toJPH(rot);

    EMotionType motion = EMotionType::Static;
    switch (this->Type)
    {
        case PhysicBodyType::RIGID_BODY:    motion = EMotionType::Dynamic;   break;
        case PhysicBodyType::KINEMATIC_BODY:motion = EMotionType::Kinematic; break;
        case PhysicBodyType::STATIC_BODY:
        default:                            motion = EMotionType::Static;    break;
    }

    JPH::ObjectLayer layer = ResolveObjectLayer(this->Type, this->CollisionGroup);

    BodyCreationSettings s(
        collider->colShape,
        RVec3(jpos.GetX(), jpos.GetY(), jpos.GetZ()),
        jrot,
        motion,
        layer
    );

    const CollisionFlag effectiveGroup = SanitizeCollisionGroup(this->CollisionGroup);
    if (effectiveGroup == CollisionFlag::COL_TRIGGER)
    {
        s.mIsSensor = true;
    }

    s.mCollisionGroup = JPH::CollisionGroup(
        PhysicsModule::getInstance()->GetCollisionFilter(),
        static_cast<JPH::CollisionGroup::GroupID>(effectiveGroup),
        static_cast<JPH::CollisionGroup::SubGroupID>(SanitizeCollisionMask(this->CollisionMask))
    );

    if (motion == EMotionType::Dynamic)
    {
        s.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        s.mMassPropertiesOverride.mMass = glm::max(0.0001f, this->Mass);
    }

    s.mFriction = glm::max(0.0f, this->Friction);
    s.mRestitution = glm::max(0.0f, this->Restitution);
    s.mLinearDamping = glm::max(0.0f, this->LinearDamping);
    s.mAngularDamping = glm::max(0.0f, this->AngularDamping);

    this->body = PhysicsModule::getInstance()->AddRigidBody(s, JPH::EActivation::Activate);

    currPos = pos; currRot = rot;
    prevPos = pos; prevRot = rot;
    hasCurr = true;

    CaptureStateSnapshot();
    lastTransformVersion = transform->GetWorldVersion();
    _QEInitialized = true;
}

void PhysicsBody::copyTransformtoGLM()
{
    if (this->body.IsInvalid() || !transform)
        return;

    RMat44 m = PhysicsModule::getInstance()->Bodies().GetCenterOfMassTransform(this->body);
    RVec3 p = m.GetTranslation();
    Quat  q = m.GetRotation().GetQuaternion();

    glm::vec3 newPos = toGLM(p);
    glm::quat newRot = toGLM(q);

    bool changed = !hasCurr
        || !closePos(currPos, newPos, posEps)
        || !closeRot(currRot, newRot, angEps);

    if (!changed)
        return;

    glm::vec3 worldScale = transform->GetWorldScale();
    glm::mat4 worldM = glm::translate(glm::mat4(1.0f), newPos)
        * glm::mat4_cast(newRot)
        * glm::scale(glm::mat4(1.0f), worldScale);

    glm::mat4 localM = worldM;
    if (auto parent = transform->GetParent())
    {
        glm::mat4 parentWorld = parent->GetWorldMatrix();
        localM = glm::inverse(parentWorld) * worldM;
    }

    applyingPhysicsTransform = true;
    transform->SetFromMatrix(localM);
    applyingPhysicsTransform = false;

    currPos = newPos;
    currRot = newRot;
    hasCurr = true;
    lastTransformVersion = transform->GetWorldVersion();
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
        glm::vec2 extents = plane->GetExtents();

        BoxShapeSettings settings(Vec3(extents.x * glm::abs(worldScale.x), thickness * glm::abs(worldScale.y), extents.y * glm::abs(worldScale.z)));
        settings.mConvexRadius = 0.0f;

        if (auto res = settings.Create(); res.IsValid())
            newShape = res.Get();
    }

    if (!newShape)
    {
        collider->colShape = nullptr;
        return;
    }

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
        PhysicsModule::getInstance()->RegisterBodyComponent(this);
        QEGameComponent::QEInit();
    }
}

void PhysicsBody::QEUpdate()
{
    if (this->Owner == nullptr) return;

    this->collider = this->Owner->GetComponentInChildren<QECollider>(true);
    this->transform = this->Owner->GetComponent<QETransform>();

    if (!this->collider)
    {
        DestroyRuntimeBody();
        previousCollider.reset();
        return;
    }

    if (this->collider && this->transform && !this->QEInitialized())
    {
        this->Initialize();
        PhysicsModule::getInstance()->RegisterBodyComponent(this);
        QEGameComponent::QEInit();
    }

    if (this->QEInitialized())
    {
        RefreshEditorState();
        UpdateTransform();
    }
}

void PhysicsBody::QEDestroy()
{
    PhysicsModule::getInstance()->UnregisterBodyComponent(this);
    DestroyRuntimeBody();
    QEGameComponent::QEDestroy();
}

void PhysicsBody::RefreshEditorState()
{
    if (!this->QEInitialized() || !transform || !collider)
        return;

    SyncShapeAndBodyToEditorState();

    const uint32_t currentVersion = transform->GetWorldVersion();
    if (!applyingPhysicsTransform && currentVersion != lastTransformVersion)
    {
        PushTransformToPhysics(true);
        lastTransformVersion = currentVersion;
    }
}

void PhysicsBody::PushTransformToPhysics(bool resetVelocities)
{
    if (body.IsInvalid() || !transform)
        return;

    auto& bodies = PhysicsModule::getInstance()->Bodies();
    const glm::vec3 worldPos = transform->GetWorldPosition();
    const glm::quat worldRot = transform->GetWorldRotation();
    bodies.SetPositionAndRotationWhenChanged(
        body,
        JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
        toJPH(worldRot),
        JPH::EActivation::Activate);

    if (resetVelocities && Type == PhysicBodyType::RIGID_BODY)
    {
        bodies.SetLinearAndAngularVelocity(body, JPH::Vec3::sZero(), JPH::Vec3::sZero());
    }

    currPos = worldPos;
    currRot = worldRot;
    hasCurr = true;
}

void PhysicsBody::SyncShapeAndBodyToEditorState()
{
    const bool colliderChanged = HasColliderDefinitionChanged();
    const bool bodyRecreationChanged = HasBodyRecreationConfigurationChanged();
    const bool bodyMaterialChanged = HasBodyMaterialConfigurationChanged();

    if (!colliderChanged && !bodyRecreationChanged && !bodyMaterialChanged)
        return;

    if (bodyRecreationChanged)
    {
        RecreateBody();
        return;
    }

    if (colliderChanged)
    {
        RebuildScaledShapeFromCollider();
        if (!collider->colShape || body.IsInvalid())
            return;

        auto& bodies = PhysicsModule::getInstance()->Bodies();
        bodies.SetShape(body, collider->colShape.GetPtr(), Type == PhysicBodyType::RIGID_BODY, JPH::EActivation::Activate);
    }

    if (bodyMaterialChanged)
    {
        ApplyRuntimeMaterialProperties();
    }

    CaptureStateSnapshot();
}

void PhysicsBody::CaptureStateSnapshot()
{
    if (!transform || !collider)
        return;

    previousCollider = collider;
    lastWorldScale = transform->GetWorldScale();
    lastColliderMargin = collider->CollisionMargin;
    lastBodyType = Type;
    lastCollisionGroup = CollisionGroup;
    lastCollisionMask = CollisionMask;
    lastMass = Mass;
    lastFriction = Friction;
    lastRestitution = Restitution;
    lastLinearDamping = LinearDamping;
    lastAngularDamping = AngularDamping;
    lastColliderVecA = glm::vec3(0.0f);
    lastColliderFloatA = 0.0f;
    lastColliderKind = DetectColliderKind(collider);

    if (auto box = std::dynamic_pointer_cast<BoxCollider>(collider))
    {
        lastColliderVecA = box->GetSize();
    }
    else if (auto sphere = std::dynamic_pointer_cast<SphereCollider>(collider))
    {
        lastColliderFloatA = sphere->GetRadius();
    }
    else if (auto capsule = std::dynamic_pointer_cast<CapsuleCollider>(collider))
    {
        lastColliderFloatA = capsule->GetRadius();
        lastColliderVecA.x = capsule->GetHeight();
    }
    else if (auto plane = std::dynamic_pointer_cast<PlaneCollider>(collider))
    {
        lastColliderFloatA = plane->GetSize();
        glm::vec2 extents = plane->GetExtents();
        lastColliderVecA = glm::vec3(extents.x, extents.y, 0.0f);
    }
}

bool PhysicsBody::HasColliderDefinitionChanged() const
{
    if (!transform || !collider)
        return false;

    if (collider != previousCollider)
        return true;

    if (DetectColliderKind(collider) != lastColliderKind)
        return true;

    if (!closePos(transform->GetWorldScale(), lastWorldScale, 0.0001f))
        return true;

    if (fabsf(collider->CollisionMargin - lastColliderMargin) > 0.0001f)
        return true;

    if (auto box = std::dynamic_pointer_cast<BoxCollider>(collider))
        return !closePos(box->GetSize(), lastColliderVecA, 0.0001f);

    if (auto sphere = std::dynamic_pointer_cast<SphereCollider>(collider))
        return fabsf(sphere->GetRadius() - lastColliderFloatA) > 0.0001f;

    if (auto capsule = std::dynamic_pointer_cast<CapsuleCollider>(collider))
        return fabsf(capsule->GetRadius() - lastColliderFloatA) > 0.0001f
            || fabsf(capsule->GetHeight() - lastColliderVecA.x) > 0.0001f;

    if (auto plane = std::dynamic_pointer_cast<PlaneCollider>(collider))
        return fabsf(plane->GetSize() - lastColliderFloatA) > 0.0001f
            || !closePos(glm::vec3(plane->GetExtents().x, plane->GetExtents().y, 0.0f), lastColliderVecA, 0.0001f);

    return false;
}

bool PhysicsBody::HasBodyRecreationConfigurationChanged() const
{
    return Type != lastBodyType
        || CollisionGroup != lastCollisionGroup
        || CollisionMask != lastCollisionMask
        || fabsf(Mass - lastMass) > 0.0001f;
}

bool PhysicsBody::HasBodyMaterialConfigurationChanged() const
{
    return fabsf(Friction - lastFriction) > 0.0001f
        || fabsf(Restitution - lastRestitution) > 0.0001f
        || fabsf(LinearDamping - lastLinearDamping) > 0.0001f
        || fabsf(AngularDamping - lastAngularDamping) > 0.0001f;
}

void PhysicsBody::ApplyRuntimeMaterialProperties()
{
    if (body.IsInvalid())
        return;

    auto& bodies = PhysicsModule::getInstance()->Bodies();
    bodies.SetFriction(body, glm::max(0.0f, Friction));
    bodies.SetRestitution(body, glm::max(0.0f, Restitution));

    JPH::BodyLockWrite lock(PhysicsModule::getInstance()->World().GetBodyLockInterface(), body);
    if (!lock.Succeeded())
        return;

    if (JPH::MotionProperties* motionProperties = lock.GetBody().GetMotionPropertiesUnchecked())
    {
        motionProperties->SetLinearDamping(glm::max(0.0f, LinearDamping));
        motionProperties->SetAngularDamping(glm::max(0.0f, AngularDamping));
    }
}

void PhysicsBody::DestroyRuntimeBody()
{
    if (!this->body.IsInvalid())
    {
        PhysicsModule::getInstance()->RemoveRigidBody(body);
        this->body = JPH::BodyID();
    }

    _QEInitialized = false;
    hasCurr = false;
}

void PhysicsBody::RecreateBody()
{
    DestroyRuntimeBody();
    Initialize();
}

CollisionFlag PhysicsBody::SanitizeCollisionGroup(CollisionFlag group)
{
    switch (group)
    {
    case COL_PLAYER:
    case COL_SCENE:
    case COL_ENEMY:
    case COL_TRIGGER:
        return group;
    case COL_DEFAULT:
    case COL_NOTHING:
    default:
        return COL_SCENE;
    }
}

unsigned int PhysicsBody::SanitizeCollisionMask(CollisionFlag mask)
{
    const unsigned int validMask = QEPhysicsCollisionMaskAll();
    unsigned int result = static_cast<unsigned int>(mask);
    if (mask == COL_ALL)
        result = validMask;

    result &= validMask;
    if (result == 0)
        result = validMask;

    return result;
}

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

