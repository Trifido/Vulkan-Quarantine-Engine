#include <TextureManager.h>

TextureManager* TextureManager::instance = nullptr;

TextureManager::TextureManager()
{
    this->AddTexture("NULL_TEXTURE", CustomTexture());
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

TextureManager* TextureManager::getInstance()
{
    if (instance == NULL)
        instance = new TextureManager();
    else
        std::cout << "Getting existing instance of Texture Manager" << std::endl;

    return instance;
}

std::shared_ptr<CustomTexture> TextureManager::GetTexture(std::string nameTexture)
{
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>>::const_iterator got = _textures.find(nameTexture);

    if (got == _textures.end())
        return nullptr;

    return _textures[nameTexture];
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
    std::unordered_map<std::string, std::shared_ptr<CustomTexture>>::const_iterator got = _textures.find(textureName);

    if (got == _textures.end())
        return false;

    return true;
}

void TextureManager::Clean()
{
    for (auto& it : _textures)
    {
        it.second->cleanup();
    }
}
