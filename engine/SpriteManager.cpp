#include "SpriteManager.h"

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    sf::Texture* texture = nullptr;

    auto it = mTextures.find(texturePath);

    if (it != mTextures.end()) {
        texture = it->second.get();
    }
    else {
        auto newTexture = std::make_unique<sf::Texture>();

        if (!newTexture->loadFromFile(texturePath)) {
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }

        mTextures[texturePath] = std::move(newTexture);
        texture = mTextures[texturePath].get();
    }

    sf::IntRect rect(
        {(int)sourceRect.topLeft.x, (int)sourceRect.topLeft.y},
        {(int)sourceRect.width, (int)sourceRect.height}
    );

    auto sprite = std::make_unique<sf::Sprite>(*texture, rect);

    SpriteID id = mSprites.size();
    mSprites.push_back(std::move(sprite));

    return id;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    if (id >= mSprites.size()) {
        throw std::out_of_range("Invalid SpriteID");
    }

    return *mSprites[id];
}

}  // namespace ECSEngine
