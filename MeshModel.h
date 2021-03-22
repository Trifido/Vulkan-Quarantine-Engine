#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Mesh.h"

class MeshModel
{
public:
	MeshModel();
	MeshModel(std::vector<Mesh> newMeshList);

	size_t getMeshCount();
	Mesh* getMesh(size_t index);

	glm::mat4 getModel();
	void setModel(glm::mat4 newModel);

	void destroyMeshModel();

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, 
		aiNode* node, const aiScene *scene, std::vector<int> matToTex);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex);

	~MeshModel();

	glm::mat4 model;
private:
	std::vector<Mesh> meshList;
};

