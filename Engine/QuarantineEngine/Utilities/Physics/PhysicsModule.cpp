#include "PhysicsModule.h"
#include <iostream>

PhysicsModule::PhysicsModule()
{
    this->collisionConfiguration = new btDefaultCollisionConfiguration();

    ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
    this->dispatcher = new btCollisionDispatcher(collisionConfiguration);

    ///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
    this->overlappingPairCache = new btDbvtBroadphase();

    ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
    this->solver = new btSequentialImpulseConstraintSolver;

    this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher, this->overlappingPairCache, this->solver, this->collisionConfiguration);

    this->dynamicsWorld->setGravity(btVector3(0, this->gravity, 0));
}

void PhysicsModule::AddRigidBody(btRigidBody* body)
{
    this->dynamicsWorld->addRigidBody(body);
}

void PhysicsModule::ComputePhysics(float deltaTime)
{
    if (this->dynamicsWorld)
    {
        this->dynamicsWorld->stepSimulation(deltaTime, 10);
    }
}

void PhysicsModule::SetGravity(float gravity)
{
    this->gravity = gravity;
    this->dynamicsWorld->setGravity(btVector3(0, this->gravity, 0));
}

PhysicsModule::~PhysicsModule()
{
    //remove the rigidbodies from the dynamics world and delete them

    if (this->dynamicsWorld)
    {
        int i;
        for (i = this->dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
        {
            this->dynamicsWorld->removeConstraint(this->dynamicsWorld->getConstraint(i));
        }
        for (i = this->dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
        {
            btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            this->dynamicsWorld->removeCollisionObject(obj);
            delete obj;
        }
    }
    //delete collision shapes
    for (int j = 0; j < this->collisionShapes.size(); j++)
    {
        btCollisionShape* shape = this->collisionShapes[j];
        delete shape;
    }
    this->collisionShapes.clear();

    delete this->dynamicsWorld;
    this->dynamicsWorld = 0;

    delete this->solver;
    this->solver = 0;

    delete this->overlappingPairCache;
    this->overlappingPairCache = 0;

    delete this->dispatcher;
    this->dispatcher = 0;

    delete this->collisionConfiguration;
    this->collisionConfiguration = 0;
}
