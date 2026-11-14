
#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

using SpriteID = size_t;

/**
 * @brief Manages textures and sprites used by the game engine.
 *
 * Caches textures by file path to avoid loading the same image
 * multiple times. Each registered sprite references a selected
 * region of a texture and is accessed through a unique SpriteID.
 *
 * AI-generated documentation.
 */
class SpriteManager {
public:
    /**
     * @brief Creates an empty sprite manager.
     *
     * AI-generated documentation.
     */
    SpriteManager() = default;

    /**
     * @brief Destroys the sprite manager and its stored resources.
     *
     * AI-generated documentation.
     */
    ~SpriteManager() = default;

    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> mTextures;
    std::vector<std::unique_ptr<sf::Sprite>> mSprites;
};

}  // namespace ECSEngine
