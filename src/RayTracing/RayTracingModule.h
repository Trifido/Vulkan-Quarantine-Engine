#pragma once
#ifndef RAYTRACING_MODULE
#define RAYTRACING_MODULE

#include "BufferManageModule.h"
#include "CommandPoolModule.h"

class RayTracingModule
{
private:
    DeviceModule*               deviceModule;
    CommandPoolModule*          commandPoolInstance;
    BufferManageModule*         bufferManageModule;
    QueueModule*                queueModule;

    VkAccelerationStructureKHR  bottomLevelAccelerationStructure;
    VkBuffer                    bottomLevelAccelerationStructureBuffer;
    VkDeviceMemory              bottomLevelAccelerationStructureBufferMemory;

    VkAccelerationStructureKHR  topLevelAccelerationStructure;
    VkBuffer                    topLevelAccelerationStructureBuffer;
    VkDeviceMemory              topLevelAccelerationStructureBufferMemory;

private:
    void initBLAS();
    void initTLAS();

public:
    RayTracingModule();
    void addModules(BufferManageModule& bufferModule, QueueModule& queueModule);
    void initRayTracing();
    void initAccelerationStructure();
};

#endif // !RAYTRACING_MODULE

