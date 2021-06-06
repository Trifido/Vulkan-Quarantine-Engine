#include "Material.h"

Material::Material()
{
    commandPoolModule = CommandPoolModule::getInstance();
    ambient = diffuse = specular = emission = glm::vec3(0.0f);
}

void Material::addAlbedo(std::string path)
{
    albedo = std::make_unique<Texture>();
    albedo->createTextureImage(path, *commandPoolModule);
}

void Material::cleanup()
{
    albedo->cleanup();
}
