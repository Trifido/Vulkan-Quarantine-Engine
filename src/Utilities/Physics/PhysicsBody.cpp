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

    if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->Mass = 1.0f;
    }
    else
    {
        this->Mass = 0.0f;
    }

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
    {
        this->collider->colShape->calculateLocalInertia(this->Mass, this->localInertia);
    }

    //Create RigidBody object
    if (this->collider->compound == nullptr)
    {
        btRigidBody::btRigidBodyConstructionInfo rbInfo(this->Mass, myMotionState, this->collider->colShape, this->localInertia);
        this->body = new btRigidBody(rbInfo);
    }
    else
    {
        btRigidBody::btRigidBodyConstructionInfo rbInfo(this->Mass, myMotionState, this->collider->compound, this->localInertia);
        this->body = new btRigidBody(rbInfo);
    }

    //Add new rigidBody to physicsModule
    auto physicsModule = PhysicsModule::getInstance();
    physicsModule->dynamicsWorld->addRigidBody(body, this->CollisionGroup, this->CollisionMask);

    if (this->Type == PhysicBodyType::STATIC_BODY)
    {
        this->body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
    }
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

    this->collider = this->Owner->GetComponent<QECollider>();
    this->transform = this->Owner->GetComponent<QETransform>();

    if (this->collider && this->transform)
    {
        this->Initialize();
        QEGameComponent::QEInit();
    }
}

void PhysicsBody::QEUpdate()
{
    this->UpdateTransform();
}

void PhysicsBody::QEDestroy()
{
    QEGameComponent::QEDestroy();
}
