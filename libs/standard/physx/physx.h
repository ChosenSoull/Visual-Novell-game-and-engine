#ifndef PHYSX_H
#define PHYSX_H

#include <PxPhysicsAPI.h>
#include <string>

class PhysXWrapper {
public:
    PhysXWrapper(const std::string& projectType); // "2d" или "3d"
    ~PhysXWrapper();

    void initialize();
    void stepSimulation(float dt);
    physx::PxRigidDynamic* createDynamicBox(float x, float y, float z, float size);
    physx::PxRigidStatic* createStaticPlane(float height);

private:
    physx::PxFoundation* mFoundation;
    physx::PxPhysics* mPhysics;
    physx::PxScene* mScene;
    physx::PxMaterial* mMaterial;
    bool is2D; // Флаг для 2D-режима
};

#endif // PHYSX_H