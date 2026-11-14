
#include "SpriteManager.h"

namespace ECSEngine {

/**
 * @brief Registers a sprite using a texture and source rectangle.
 *
 * @param texturePath Path to the texture image file.
 * @param sourceRect Rectangle defining the sprite's region in the texture.
 * @return SpriteID assigned to the newly registered sprite.
 *
 * Checks whether the texture has already been loaded and reuses
 * it when available. Otherwise, loads and caches the texture.
 * Creates a sprite from the specified region and stores it for
 * future access. Throws std::runtime_error if loading fails.
 *
 * AI-generated documentation.
 */
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

/**
 * @brief Retrieves a previously registered sprite.
 *
 * @param id SpriteID of the requested sprite.
 * @return Reference to the corresponding SFML sprite.
 *
 * Looks up the sprite using its index in the sprite storage.
 * Throws std::out_of_range if the ID is invalid.
 *
 * AI-generated documentation.
 */
sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    if (id >= mSprites.size()) {
        throw std::out_of_range("Invalid SpriteID");
    }

    return *mSprites[id];
}

}  // namespace ECSEngine
