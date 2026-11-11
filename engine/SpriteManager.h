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

class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> mTextures;
    std::vector<std::unique_ptr<sf::Sprite>> mSprites;
};

}  // namespace ECSEngine
