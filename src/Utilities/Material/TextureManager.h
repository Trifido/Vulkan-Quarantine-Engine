#pragma once

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <CustomTexture.h>
#include <unordered_map>
#include <QESingleton.h>

class TextureManager : public QESingleton<TextureManager>
{
private:
    friend class QESingleton<TextureManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>> _textures;

private:
    TextureManager();
    std::string CheckName(std::string textureName);

public:
    std::shared_ptr<CustomTexture> GetTexture(std::string nameTexture);
    std::string AddTexture(std::string textureName, std::shared_ptr<CustomTexture> texture_ptr);
    std::string AddTexture(std::string textureName, CustomTexture texture);
    bool Exists(std::string textureName);
    void Clean();
    void CleanLastResources();
};

#endif
