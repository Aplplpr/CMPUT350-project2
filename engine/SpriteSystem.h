#pragma once

#include <bitset>
#include <unordered_map>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "LocationComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();
        auto& spriteManager = engine->GetSpriteManager();
        auto& windowManager = engine->GetWindowManager();

        auto& sprites = entityManager.template GetComponentStorage<SpriteComponent>();

        for (size_t i = 0; i < sprites.mStorage.size(); i++) {
            EntityID id = sprites.mOwners[i];

            if (!entityManager.IsValidEntity(id)) continue;

            auto& component = sprites.mStorage[i];
            Rect bounds = component.bounds;

            if (entityManager.template HasComponent<LocationComponent>(id)) {
                auto& location = entityManager.template GetComponent<LocationComponent>(id);
                bounds.topLeft.x += location.loc.x;
                bounds.topLeft.y += location.loc.y;
            }

            if (component.worldSpace) {
                bounds = windowManager.WorldToWindow(bounds);
            }

            sf::Sprite& sprite = spriteManager.GetSprite(component.textureID);
            sf::IntRect textureRect = sprite.getTextureRect();

            if (textureRect.size.x == 0 || textureRect.size.y == 0) continue;

            float scaleX = bounds.width / textureRect.size.x;
            float scaleY = bounds.height / textureRect.size.y;

            sprite.setPosition({bounds.topLeft.x, bounds.topLeft.y});
            sprite.setScale({scaleX, scaleY});

            windowManager.GetWindow()->draw(sprite);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
