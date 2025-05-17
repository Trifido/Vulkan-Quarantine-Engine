#include "RayTracingModule.h"


void RayTracingModule::initBLAS()
{
    /*
    PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(deviceModule->device, "vkGetAccelerationStructureBuildSizesKHR");
    PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(deviceModule->device, "vkCreateAccelerationStructureKHR");
    PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(deviceModule->device, "vkGetBufferDeviceAddressKHR");
    PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(deviceModule->device, "vkCmdBuildAccelerationStructuresKHR");

    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo = {};
    vertexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    vertexBufferDeviceAddressInfo.buffer = bufferManageModule->vertexBuffer;

    VkDeviceAddress vertexBufferAddress = pvkGetBufferDeviceAddressKHR(deviceModule->device, &vertexBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR vertexDeviceOrHostAddressConst = {};
    vertexDeviceOrHostAddressConst.deviceAddress = vertexBufferAddress;

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo = {};
    indexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    indexBufferDeviceAddressInfo.buffer = bufferManageModule->indexBuffer;

    VkDeviceAddress indexBufferAddress = pvkGetBufferDeviceAddressKHR(deviceModule->device, &indexBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR indexDeviceOrHostAddressConst = {};
    indexDeviceOrHostAddressConst.deviceAddress = indexBufferAddress;

    VkAccelerationStructureGeometryTrianglesDataKHR accelerationStructureGeometryTrianglesData = {};
    accelerationStructureGeometryTrianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometryTrianglesData.pNext = NULL;
    accelerationStructureGeometryTrianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometryTrianglesData.vertexData = vertexDeviceOrHostAddressConst;
    accelerationStructureGeometryTrianglesData.vertexStride = sizeof(float) * 3;
    accelerationStructureGeometryTrianglesData.maxVertex = bufferManageModule->getGeometryData()->numVertices;
    accelerationStructureGeometryTrianglesData.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometryTrianglesData.indexData = indexDeviceOrHostAddressConst;
    accelerationStructureGeometryTrianglesData.transformData = {};

    VkAccelerationStructureGeometryDataKHR accelerationStructureGeometryData = {};
    accelerationStructureGeometryData.triangles = accelerationStructureGeometryTrianglesData;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry;
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.pNext = NULL;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry = accelerationStructureGeometryData;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        accelerationStructureBuildGeometryInfo.pNext = NULL;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = 0;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationStructureBuildGeometryInfo.ppGeometries = NULL;
    accelerationStructureBuildGeometryInfo.scratchData = {};

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    accelerationStructureBuildSizesInfo.pNext = NULL;
    accelerationStructureBuildSizesInfo.accelerationStructureSize = 0;
    accelerationStructureBuildSizesInfo.updateScratchSize = 0;
    accelerationStructureBuildSizesInfo.buildScratchSize = 0;

    pvkGetAccelerationStructureBuildSizesKHR(deviceModule->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR,
        &accelerationStructureBuildGeometryInfo,
        &accelerationStructureBuildGeometryInfo.geometryCount,
        &accelerationStructureBuildSizesInfo);

    BufferManageModule::createBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bottomLevelAccelerationStructureBuffer, bottomLevelAccelerationStructureBufferMemory, *deviceModule);

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;
    BufferManageModule::createBuffer(accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratchBuffer,
        scratchBufferMemory, *deviceModule);

    VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo = {};
    scratchBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferDeviceAddressInfo.buffer = scratchBuffer;

    VkDeviceAddress scratchBufferAddress = pvkGetBufferDeviceAddressKHR(deviceModule->device, &scratchBufferDeviceAddressInfo);

    VkDeviceOrHostAddressKHR scratchDeviceOrHostAddress = {};
    scratchDeviceOrHostAddress.deviceAddress = scratchBufferAddress;

    accelerationStructureBuildGeometryInfo.scratchData = scratchDeviceOrHostAddress;

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.pNext = NULL;
    accelerationStructureCreateInfo.createFlags = 0;
    accelerationStructureCreateInfo.buffer = bottomLevelAccelerationStructureBuffer;
    accelerationStructureCreateInfo.offset = 0;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureCreateInfo.deviceAddress = VK_NULL_HANDLE;

    pvkCreateAccelerationStructureKHR(deviceModule->device, &accelerationStructureCreateInfo, NULL, &bottomLevelAccelerationStructure);

    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = bottomLevelAccelerationStructure;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo_data = {};
    accelerationStructureBuildRangeInfo_data.primitiveCount = bufferManageModule->getGeometryData()->numFaces;
    accelerationStructureBuildRangeInfo_data.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo_data.firstVertex = 0;
    accelerationStructureBuildRangeInfo_data.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* accelerationStructureBuildRangeInfo = &accelerationStructureBuildRangeInfo_data;
    const VkAccelerationStructureBuildRangeInfoKHR** accelerationStructureBuildRangeInfos = &accelerationStructureBuildRangeInfo;

    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = commandPoolInstance->getCommandPool();
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(deviceModule->device, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    pvkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queueModule->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueModule->computeQueue);

    vkFreeCommandBuffers(deviceModule->device, commandPoolInstance->getCommandPool(), 1, &commandBuffer);

    vkDestroyBuffer(deviceModule->device, scratchBuffer, NULL);
    vkFreeMemory(deviceModule->device, scratchBufferMemory, NULL);
    */
}

void RayTracingModule::initTLAS()
{
    /*
    PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(deviceModule->device, "vkGetAccelerationStructureBuildSizesKHR");
    PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(deviceModule->device, "vkCreateAccelerationStructureKHR");
    PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(deviceModule->device, "vkGetBufferDeviceAddressKHR");
    PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(deviceModule->device, "vkCmdBuildAccelerationStructuresKHR");
    PFN_vkGetAccelerationStructureDeviceAddressKHR pvkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(deviceModule->device, "vkGetAccelerationStructureDeviceAddressKHR");

    VkTransformMatrixKHR transformMatrix = {};
    transformMatrix.matrix[0][0] = 1.0;
    transformMatrix.matrix[1][1] = 1.0;
    transformMatrix.matrix[2][2] = 1.0;

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = bottomLevelAccelerationStructure;

    VkDeviceAddress accelerationStructureDeviceAddress = pvkGetAccelerationStructureDeviceAddressKHR(deviceModule->device, &accelerationStructureDeviceAddressInfo);

    VkAccelerationStructureInstanceKHR geometryInstance = {};
    geometryInstance.transform = transformMatrix;
    geometryInstance.instanceCustomIndex = 0;
    geometryInstance.mask = 0xFF;
    geometryInstance.instanceShaderBindingTableRecordOffset = 0;
    geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    geometryInstance.accelerationStructureReference = accelerationStructureDeviceAddress;

    VkDeviceSize geometryInstanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR);

    VkBuffer geometryInstanceStagingBuffer;
    VkDeviceMemory geometryInstanceStagingBufferMemory;
    BufferManageModule::createBuffer(geometryInstanceBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, geometryInstanceStagingBuffer, geometryInstanceStagingBufferMemory, *deviceModule);

    void* geometryInstanceData;
    vkMapMemory(deviceModule->device, geometryInstanceStagingBufferMemory, 0, geometryInstanceBufferSize, 0, &geometryInstanceData);
    memcpy(geometryInstanceData, &geometryInstance, geometryInstanceBufferSize);
    vkUnmapMemory(deviceModule->device, geometryInstanceStagingBufferMemory);

    VkBuffer geometryInstanceBuffer;
    VkDeviceMemory geometryInstanceBufferMemory;
    BufferManageModule::createBuffer(geometryInstanceBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, geometryInstanceBuffer, geometryInstanceBufferMemory, *deviceModule);

    bufferManageModule->copyBuffer(geometryInstanceStagingBuffer, geometryInstanceBuffer, geometryInstanceBufferSize);

    vkDestroyBuffer(deviceModule->device, geometryInstanceStagingBuffer, NULL);
    vkFreeMemory(deviceModule->device, geometryInstanceStagingBufferMemory, NULL);

    VkBufferDeviceAddressInfo geometryInstanceBufferDeviceAddressInfo = {};
    geometryInstanceBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    geometryInstanceBufferDeviceAddressInfo.buffer = geometryInstanceBuffer;

    VkDeviceAddress geometryInstanceBufferAddress = pvkGetBufferDeviceAddressKHR(deviceModule->device, &geometryInstanceBufferDeviceAddressInfo);

    VkDeviceOrHostAddressConstKHR geometryInstanceDeviceOrHostAddressConst = {};
    geometryInstanceDeviceOrHostAddressConst.deviceAddress = geometryInstanceBufferAddress;

    VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData = {};
    accelerationStructureGeometryInstancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometryInstancesData.pNext = NULL;
    accelerationStructureGeometryInstancesData.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometryInstancesData.data = geometryInstanceDeviceOrHostAddressConst;

    VkAccelerationStructureGeometryDataKHR accelerationStructureGeometryData = {};
    accelerationStructureGeometryData.instances = accelerationStructureGeometryInstancesData;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.pNext = NULL;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.geometry = accelerationStructureGeometryData;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = NULL;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationStructureBuildGeometryInfo.ppGeometries = NULL;
    accelerationStructureBuildGeometryInfo.scratchData = {};

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    accelerationStructureBuildSizesInfo.pNext = NULL;
    accelerationStructureBuildSizesInfo.accelerationStructureSize = 0;
    accelerationStructureBuildSizesInfo.updateScratchSize = 0;
    accelerationStructureBuildSizesInfo.buildScratchSize = 0;

    pvkGetAccelerationStructureBuildSizesKHR(deviceModule->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR,
        &accelerationStructureBuildGeometryInfo,
        &accelerationStructureBuildGeometryInfo.geometryCount,
        &accelerationStructureBuildSizesInfo);

    bufferManageModule->createBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, topLevelAccelerationStructureBuffer, topLevelAccelerationStructureBufferMemory, *deviceModule);

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;
    bufferManageModule->createBuffer(
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratchBuffer,
        scratchBufferMemory, *deviceModule);


    VkBufferDeviceAddressInfo scratchBufferDeviceAddressInfo = {};
    scratchBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchBufferDeviceAddressInfo.buffer = scratchBuffer;

    VkDeviceAddress scratchBufferAddress = pvkGetBufferDeviceAddressKHR(deviceModule->device, &scratchBufferDeviceAddressInfo);

    VkDeviceOrHostAddressKHR scratchDeviceOrHostAddress = {};
    scratchDeviceOrHostAddress.deviceAddress = scratchBufferAddress;

    accelerationStructureBuildGeometryInfo.scratchData = scratchDeviceOrHostAddress;

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.pNext = NULL;
    accelerationStructureCreateInfo.createFlags = 0;
    accelerationStructureCreateInfo.buffer = topLevelAccelerationStructureBuffer;
    accelerationStructureCreateInfo.offset = 0;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureCreateInfo.deviceAddress = VK_NULL_HANDLE;

    pvkCreateAccelerationStructureKHR(deviceModule->device, &accelerationStructureCreateInfo, NULL, &topLevelAccelerationStructure);

    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = topLevelAccelerationStructure;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo_data = {};
    accelerationStructureBuildRangeInfo_data.primitiveCount = 1;
    accelerationStructureBuildRangeInfo_data.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo_data.firstVertex = 0;
    accelerationStructureBuildRangeInfo_data.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* accelerationStructureBuildRangeInfo = &accelerationStructureBuildRangeInfo_data;
    const VkAccelerationStructureBuildRangeInfoKHR** accelerationStructureBuildRangeInfos = &accelerationStructureBuildRangeInfo;

    VkCommandBufferAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocateInfo.commandPool = commandPoolInstance->getCommandPool();
    bufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(deviceModule->device, &bufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    pvkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queueModule->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueModule->computeQueue);

    vkFreeCommandBuffers(deviceModule->device, commandPoolInstance->getCommandPool(), 1, &commandBuffer);

    vkDestroyBuffer(deviceModule->device, scratchBuffer, NULL);
    vkFreeMemory(deviceModule->device, scratchBufferMemory, NULL);
    */
}

RayTracingModule::RayTracingModule()
{
    deviceModule = DeviceModule::getInstance();
    commandPoolInstance = CommandPoolModule::getInstance();
}

void RayTracingModule::addModules(BufferManageModule& bufferModule, QueueModule& newQueueModule)
{
    bufferManageModule = &bufferModule;
    this->queueModule = &newQueueModule;
}

void RayTracingModule::initRayTracing()
{
    // Requesting ray tracing properties
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProps = {};
    pipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 props = {};
    props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props.pNext = &pipelineProps;

    vkGetPhysicalDeviceProperties2(deviceModule->physicalDevice, &props);

    initAccelerationStructure();
}

void RayTracingModule::initAccelerationStructure()
{
    initBLAS();
    initTLAS();
}
