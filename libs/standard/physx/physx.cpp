#include "physx.h"
#include <iostream>

using namespace physx;

PhysXWrapper::PhysXWrapper(const std::string& projectType) : mFoundation(nullptr), mPhysics(nullptr), mScene(nullptr), mMaterial(nullptr) {
    is2D = (projectType == "2d");
}

PhysXWrapper::~PhysXWrapper() {
    if (mScene) mScene->release();
    if (mPhysics) mPhysics->release();
    if (mFoundation) mFoundation->release();
}

void PhysXWrapper::initialize() {
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, PxDefaultAllocator(), PxDefaultErrorCallback());
    if (!mFoundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return;
    }

    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale());
    if (!mPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return;
    }

    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    if (is2D) {
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    }
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    mScene = mPhysics->createScene(sceneDesc);
    if (!mScene) {
        std::cerr << "createScene failed!" << std::endl;
        return;
    }

    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);
}

void PhysXWrapper::stepSimulation(float dt) {
    if (mScene) {
        mScene->simulate(dt);
        mScene->fetchResults(true);
    }
}

PxRigidDynamic* PhysXWrapper::createDynamicBox(float x, float y, float z, float size) {
    PxTransform transform(PxVec3(x, y, is2D ? 0.0f : z));
    PxBoxGeometry box(size, size, is2D ? 0.1f : size);
    PxRigidDynamic* body = mPhysics->createRigidDynamic(transform);
    body->createShape(box, *mMaterial);
    mScene->addActor(*body);

    if (is2D) {
        body->setRigidDynamicLockFlags(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z | PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y);
    }

    return body;
}

PxRigidStatic* PhysXWrapper::createStaticPlane(float height) {
    PxTransform transform(PxVec3(0.0f, height, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
    PxRigidStatic* plane = mPhysics->createRigidStatic(transform);
    plane->createShape(PxPlaneGeometry(), *mMaterial);
    mScene->addActor(*plane);
    return plane;
}