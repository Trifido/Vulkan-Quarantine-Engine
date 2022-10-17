#pragma once

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <CustomTexture.h>
#include <unordered_map>

class TextureManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>> _textures;
    static TextureManager* instance;

private:
    std::string CheckName(std::string textureName);

public:
    TextureManager();
    static TextureManager* getInstance();
    std::shared_ptr<CustomTexture> GetTexture(std::string nameTexture);
    std::string AddTexture(std::string textureName, std::shared_ptr<CustomTexture> texture_ptr);
    std::string AddTexture(std::string textureName, CustomTexture texture);
    bool Exists(std::string textureName);
    void Clean();
};

#endif
