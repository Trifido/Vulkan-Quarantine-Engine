#include <TextureManager.h>
#include <CustomTexture.h>
#include <algorithm>
#include <cctype>

TextureManager::TextureManager()
{
    this->AddTexture("NULL_TEXTURE", CustomTexture());
}

std::string TextureManager::NormalizeTextureKey(const std::string& rawPath)
{
    if (rawPath.empty())
        return "";

    std::filesystem::path p(rawPath);
    p = p.lexically_normal();
    std::string key = p.generic_string();

#ifdef _WIN32
    std::transform(key.begin(), key.end(), key.begin(),
        [](unsigned char c) { return (unsigned char)std::tolower(c); });
#endif

    return key;
}

std::string TextureManager::MakeKey(const std::string& rawPath, QEColorSpace cs)
{
    std::string k = NormalizeTextureKey(rawPath);
    k += (cs == QEColorSpace::SRGB) ? "|srgb" : "|lin";
    return k;
}

std::string TextureManager::CheckName(std::string textureName)
{
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>>::const_iterator got;

    std::string newName = textureName;
    unsigned int id = 0;

    do
    {
        got = _textures.find(newName);

        if (got != _textures.end())
        {
            id++;
            newName = textureName + "_" + std::to_string(id);
        }
    } while (got != _textures.end());

    return newName;
}

std::shared_ptr<CustomTexture> TextureManager::GetTexture(std::string nameTexture)
{
    auto got = _textures.find(nameTexture);
    if (got == _textures.end())
        return nullptr;

    return got->second;
}

std::string TextureManager::AddTexture(std::string textureName, std::shared_ptr<CustomTexture> texture_ptr)
{
    std::string name = CheckName(textureName);
    _textures[name] = texture_ptr;
    return name;
}

std::string TextureManager::AddTexture(std::string textureName, CustomTexture texture)
{
    std::string name = CheckName(textureName);
    _textures[name] = std::make_shared<CustomTexture>(texture);
    return name;
}

bool TextureManager::Exists(std::string textureName)
{
    return _textures.find(textureName) != _textures.end();
}

void TextureManager::Clean()
{
    for (auto& it : _textures)
        it.second->cleanup();
}

void TextureManager::CleanLastResources()
{
    _textures.clear();
    _pathCache.clear();
    //this->AddTexture("NULL_TEXTURE", CustomTexture());
}

std::shared_ptr<CustomTexture> TextureManager::GetOrLoadTextureByPath(const std::string& rawPath, QEColorSpace cs)
{
    if (rawPath.empty() || rawPath == "NULL_TEXTURE")
        return this->GetTexture("NULL_TEXTURE");

    if (!rawPath.empty() && rawPath[0] == '*')
        return this->GetTexture("NULL_TEXTURE");

    const std::string normalizedPath = NormalizeTextureKey(rawPath);
    const std::string key = MakeKey(normalizedPath, cs);

    auto it = _pathCache.find(key);
    if (it != _pathCache.end() && it->second)
        return it->second;

    // Ojo: type aquí NO es semantic; usa NULL_TYPE (2D normal)
    auto tex = std::make_shared<CustomTexture>(normalizedPath, TEXTURE_TYPE::NULL_TYPE, cs);

    // Guardar bajo el key (no bajo la ruta) para que no colisione srgb/lin
    _textures[key] = tex;
    _pathCache[key] = tex;
    return tex;
}
