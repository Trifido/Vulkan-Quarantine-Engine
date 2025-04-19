#include "PhysicBody.h"
#include <LinearMath/btVector3.h>
#include <PhysicsModule.h>

PhysicBody::PhysicBody()
{
    this->UpdateType(PhysicBodyType::STATIC_BODY);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
    this->CollisionMask = CollisionFlag::COL_NOTHING;
}

PhysicBody::PhysicBody(const PhysicBodyType& type)
{
    this->UpdateType(type);
    this->CollisionGroup = CollisionFlag::COL_NOTHING;
    this->CollisionMask = CollisionFlag::COL_NOTHING;
}

void PhysicBody::UpdateType(const PhysicBodyType &type)
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

void PhysicBody::UpdateInertia(const glm::vec3 &localInertia)
{
    this->Inertia = localInertia;
    this->localInertia[0] = this->Inertia.x;
    this->localInertia[1] = this->Inertia.y;
    this->localInertia[2] = this->Inertia.z;
}

void PhysicBody::UpdateTransform()
{
    if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->copyTransformtoGLM();
    }
}


void PhysicBody::Initialize(std::shared_ptr<Transform> transform_ptr, std::shared_ptr<Collider> collider_ptr)
{
    this->transform = transform_ptr;
    this->collider = collider_ptr;

    btTransform startTransform;
    startTransform.setIdentity();

    glm::vec3 scale = this->transform->Scale;
    collider_ptr->colShape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

    glm::vec3 position = this->transform->Position;
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));

    float yaw = this->transform->RadiansRotation.x;
    float pitch = this->transform->RadiansRotation.y;
    float roll = this->transform->RadiansRotation.z;
    btQuaternion rotation(yaw, pitch, roll);
    startTransform.setRotation(rotation);

    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);

    if (this->Type == PhysicBodyType::RIGID_BODY)
    {
        this->collider->colShape->calculateLocalInertia(this->Mass, this->localInertia);
    }

    //Create RigidBody object
    btRigidBody::btRigidBodyConstructionInfo rbInfo(this->Mass, myMotionState, this->collider->colShape, this->localInertia);
    this->body = new btRigidBody(rbInfo);

    //Add new rigidBody to physicsModule
    auto physicsModule = PhysicsModule::getInstance();
    physicsModule->dynamicsWorld->addRigidBody(body, this->CollisionGroup, this->CollisionMask);

    if (this->Type == PhysicBodyType::STATIC_BODY)
    {
        this->body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
    }
}

void PhysicBody::copyTransformtoGLM()
{
    btTransform trans;
    if (this->body && this->body->getMotionState())
    {
        this->body->getMotionState()->getWorldTransform(trans);

        //Update glm::matrix
        this->transform->SetModel(bulletToGlm(trans));
    }
}

glm::mat4 PhysicBody::bulletToGlm(const btTransform& t)
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

btTransform PhysicBody::glmToBullet(const glm::mat4& m)
{
    glm::mat3 m3(m);
    return btTransform(glmToBullet(m3), glmToBullet(glm::vec3(m[3][0], m[3][1], m[3][2])));
}

glm::vec3 PhysicBody::bulletToGlm(const btVector3& v) { return glm::vec3(v.getX(), v.getY(), v.getZ()); }
glm::quat PhysicBody::bulletToGlm(const btQuaternion& q) { return glm::quat(q.getW(), q.getX(), q.getY(), q.getZ()); }

btVector3 PhysicBody::glmToBullet(const glm::vec3& v) { return btVector3(v.x, v.y, v.z); }
btMatrix3x3 PhysicBody::glmToBullet(const glm::mat3& m) { return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]); }
btQuaternion PhysicBody::glmToBullet(const glm::quat& q) { return btQuaternion(q.x, q.y, q.z, q.w); }


