#include <PhysicsBody.h>
#include <LinearMath/btVector3.h>
#include <PhysicsModule.h>
#include <QEGameObject.h>

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

void PhysicsBody::UpdateInertia(const glm::vec3 &localInertia)
{
    this->Inertia = localInertia;
    this->localInertia[0] = this->Inertia.x;
    this->localInertia[1] = this->Inertia.y;
    this->localInertia[2] = this->Inertia.z;
}

void PhysicsBody::UpdateTransform()
{
    if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->copyTransformtoGLM();
    }
}

inline bool PhysicsBody::closePos(const btVector3& a, const btVector3& b, btScalar eps)
{
    return (a - b).length2() <= eps * eps;
}

inline bool PhysicsBody::closeRot(const btQuaternion& a, btQuaternion b, btScalar angleEpsRad)
{
    if (a.dot(b) < 0) b = btQuaternion(-b.x(), -b.y(), -b.z(), -b.w());
    btQuaternion d = a.inverse() * b;
    return btFabs(d.getAngle()) <= angleEpsRad;
}

void PhysicsBody::Initialize()
{
    btTransform startTransform;
    startTransform.setIdentity();

    glm::vec3 scale = this->transform->GetWorldScale();
    this->collider->colShape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

    glm::vec3 position = this->transform->GetWorldPosition();
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));

    auto quat = this->transform->GetWorldRotation();
    btQuaternion btQuat(quat.x, quat.y, quat.z, quat.w);
    startTransform.setRotation(btQuat);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);

    if (this->Type == PhysicBodyType::RIGID_BODY)
        this->collider->colShape->calculateLocalInertia(this->Mass, this->localInertia);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        this->Mass,
        myMotionState,
        (this->collider->compound ? (btCollisionShape*)this->collider->compound : (btCollisionShape*)this->collider->colShape),
        this->localInertia
    );

    this->body = new btRigidBody(rbInfo);

    switch (this->Type)
    {
        default:
        case PhysicBodyType::STATIC_BODY:
            this->body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
            break;
        case PhysicBodyType::RIGID_BODY:
            this->body->setActivationState(DISABLE_DEACTIVATION);
            this->body->setSleepingThresholds(0.f, 0.f);
            break;
        case PhysicBodyType::KINEMATIC_BODY:
            body->setCollisionFlags(body->getCollisionFlags()
                | btCollisionObject::CF_KINEMATIC_OBJECT
                | btCollisionObject::CF_CHARACTER_OBJECT); // opcional
            body->setActivationState(DISABLE_DEACTIVATION);
            body->setMassProps(0, btVector3(0, 0, 0));
            body->setLinearVelocity(btVector3(0, 0, 0)); // no usar velocities del solver
            body->setAngularVelocity(btVector3(0, 0, 0));
            break;
    }

    // Registra en el mundo
    auto physicsModule = PhysicsModule::getInstance();
    physicsModule->dynamicsWorld->addRigidBody(this->body, this->CollisionGroup, this->CollisionMask);

    // Inicializa prev/curr con el transform inicial
    currTrans = startTransform;
    prevTrans = startTransform;
    hasCurr = true;

    // Para tu lógica previa basada en lastTrans:
    lastTrans = startTransform;
    hasLastTrans = true;
}

void PhysicsBody::copyTransformtoGLM()
{
    if (!this->body) return;

    if (!this->body->isKinematicObject() && !this->body->isActive())
        return;

    btTransform trans = this->body->getInterpolationWorldTransform();

    const btVector3& newPos = trans.getOrigin();
    const btQuaternion& newRot = trans.getRotation();

    bool changed = !hasLastTrans
        || !closePos(lastTrans.getOrigin(), newPos, posEps)
        || !closeRot(lastTrans.getRotation(), newRot, angEps);

    if (!changed) return;

    glm::mat4 m = bulletToGlm(trans);
    this->transform->SetFromMatrix(m);

    lastTrans = trans;
    hasLastTrans = true;
}

glm::mat4 PhysicsBody::bulletToGlm(const btTransform& t)
{
    glm::mat4 m(0.0f);
    const btMatrix3x3& basis = t.getBasis();
    // rotation
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            m[c][r] = basis[r][c];
        }
    }
    // traslation
    btVector3 origin = t.getOrigin();
    m[3][0] = origin.getX();
    m[3][1] = origin.getY();
    m[3][2] = origin.getZ();
    // unit scale
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][3] = 1.0f;
    return m;
}

btTransform PhysicsBody::glmToBullet(const glm::mat4& m)
{
    glm::mat3 m3(m);
    return btTransform(glmToBullet(m3), glmToBullet(glm::vec3(m[3][0], m[3][1], m[3][2])));
}

glm::vec3 PhysicsBody::bulletToGlm(const btVector3& v) { return glm::vec3(v.getX(), v.getY(), v.getZ()); }
glm::quat PhysicsBody::bulletToGlm(const btQuaternion& q) { return glm::quat(q.getW(), q.getX(), q.getY(), q.getZ()); }

btVector3 PhysicsBody::glmToBullet(const glm::vec3& v) { return btVector3(v.x, v.y, v.z); }
btMatrix3x3 PhysicsBody::glmToBullet(const glm::mat3& m) { return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]); }
btQuaternion PhysicsBody::glmToBullet(const glm::quat& q) { return btQuaternion(q.x, q.y, q.z, q.w); }

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
    if (!this->body) return;

    if (this->Type == PhysicBodyType::KINEMATIC_BODY)
    {
        glm::mat4 wm = transform->GetWorldMatrix();
        btTransform t = glmToBullet(wm);
        if (body->getMotionState()) body->getMotionState()->setWorldTransform(t);
        body->setWorldTransform(t);
    }
    else if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->copyTransformtoGLM();
    }
}

void PhysicsBody::QEDestroy()
{
    QEGameComponent::QEDestroy();
}


void PhysicsBody::InitializeFromTransform()
{
    glm::mat4 wm = this->transform->GetWorldMatrix();
    currTrans = glmToBullet(wm);
    prevTrans = currTrans;
    hasCurr = true;
}

void PhysicsBody::SnapshotPrev()
{
    if (!hasCurr) InitializeFromTransform();
    prevTrans = currTrans;
}

void PhysicsBody::FetchCurrFromBullet()
{
    if (!this->body) return;

    // Si usas MotionState (sí), el estado "actual" del step está en getWorldTransform()
    btTransform t;
    this->body->getMotionState()->getWorldTransform(t);
    currTrans = t;
    hasCurr = true;

    // (Opcional pero recomendable) Mantén sincronizado el QETransform con "curr"
    // para lógica no interpolada que lea del transform:
    glm::mat4 m = bulletToGlm(currTrans);
    this->transform->SetFromMatrix(m);

    // Guarda también para copy/epsilon, si lo sigues usando:
    lastTrans = currTrans;
    hasLastTrans = true;
}

void PhysicsBody::GetInterpolated(float alpha, glm::vec3& outPos, glm::quat& outRot) const
{
    // Asegura que existe curr; si no, cae a identity
    btVector3 p0 = prevTrans.getOrigin();
    btVector3 p1 = currTrans.getOrigin();
    btQuaternion q0 = prevTrans.getRotation();
    btQuaternion q1 = currTrans.getRotation();

    btVector3 p = p0.lerp(p1, alpha);
    btQuaternion q = q0.slerp(q1, alpha);

    outPos = glm::vec3(p.x(), p.y(), p.z());
    outRot = glm::quat(q.w(), q.x(), q.y(), q.z()); 
}

glm::mat4 PhysicsBody::GetInterpolatedMatrix(float alpha) const
{
    glm::vec3 pos; glm::quat rot;
    GetInterpolated(alpha, pos, rot);
    return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
}
