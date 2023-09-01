#pragma once

#ifndef COMPUTE_NODE_MANAGER_H
#define COMPUTE_NODE_MANAGER_H

#include <unordered_map>
#include <memory>
#include "ComputeNode.h"

class ComputeNodeManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ComputeNode>> _computeNodes;

public:
    static ComputeNodeManager* instance;

private:
    std::string CheckName(std::string nameComputeNode);

public:
    ComputeNodeManager();
    static ComputeNodeManager* getInstance();
    void ComputeNodeManager::ResetInstance();
    void InitializeComputeNodes();
    void AddComputeNode(std::string& nameComputeNode, ComputeNode mat);
    void AddComputeNode(const char* nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr);
    void AddComputeNode(std::string& nameComputeNode, std::shared_ptr<ComputeNode> mat_ptr);
    std::shared_ptr<ComputeNode> GetComputeNode(std::string nameComputeNode);
    void RecordComputeNodes(VkCommandBuffer commandBuffer, uint32_t currentFrame);
    void CleanLastResources();
};

#endif // !COMPUTE_NODE_MANAGER_H
