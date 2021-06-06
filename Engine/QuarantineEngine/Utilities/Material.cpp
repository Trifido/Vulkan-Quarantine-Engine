#include "Material.h"

Material::Material()
{
    ambient = diffuse = specular = emission = glm::vec3(0.0f);
}

void Material::addAlbedo(std::string path, VkCommandPool& commandPool)
{
    albedo = std::make_unique<Texture>();
    albedo->createTextureImage(path, commandPool);
}

void Material::cleanup()
{
    albedo->cleanup();
}
