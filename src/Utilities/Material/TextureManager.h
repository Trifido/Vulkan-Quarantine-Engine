#pragma once

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <CustomTexture.h>
#include <unordered_map>
#include <QESingleton.h>
#include <filesystem>

class TextureManager : public QESingleton<TextureManager>
{
private:
    friend class QESingleton<TextureManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>> _textures;
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>> _pathCache;

private:
    TextureManager();
    static std::string NormalizeTextureKey(const std::string& rawPath);
    std::string CheckName(std::string textureName);
    static std::string MakeKey(const std::string& rawPath, QEColorSpace cs);

public:
    std::shared_ptr<CustomTexture> GetTexture(std::string nameTexture);
    std::string AddTexture(std::string textureName, std::shared_ptr<CustomTexture> texture_ptr);
    std::string AddTexture(std::string textureName, CustomTexture texture);
    bool Exists(std::string textureName);
    void Clean();
    void CleanLastResources();

    std::shared_ptr<CustomTexture> GetOrLoadTextureByPath(const std::string& rawPath, QEColorSpace cs);
};

#endif
