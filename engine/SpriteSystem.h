#pragma once

#include <bitset>
#include <unordered_map>
#include <cmath>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "CollisionComponent.h"

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

            // Update optional sprite animation
            if (component.animated &&
                entityManager.template HasComponent<MovementComponent>(id) &&
                entityManager.template HasComponent<CollisionComponent>(id)) {

                auto& movement =
                    entityManager.template GetComponent<MovementComponent>(id);
                auto& collision =
                    entityManager.template GetComponent<CollisionComponent>(id);

                // Keep the last horizontal facing direction
                if (movement.velocity.x < -1.0f) {
                    component.facingLeft = true;
                }
                else if (movement.velocity.x > 1.0f) {
                    component.facingLeft = false;
                }

                if (!collision.onGround) {
                    // Jumping or falling
                    component.textureID = component.jumpSpriteID;
                    component.animationTimer = 0;
                }
                else if (std::abs(movement.velocity.x) > 10.0f) {
                    // Walking
                    component.animationTimer++;

                    if ((component.animationTimer / 9) % 2 == 0) {
                        component.textureID = component.walkSpriteID1;
                    }
                    else {
                        component.textureID = component.walkSpriteID2;
                    }
                }
                else {
                    // Idle
                    component.textureID = component.idleSpriteID;
                    component.animationTimer = 0;
                }
            }

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

            if (component.animated && component.facingLeft) {
                sprite.setPosition({bounds.topLeft.x + bounds.width, bounds.topLeft.y});
                sprite.setScale({-scaleX, scaleY});
            }
            else {
                sprite.setPosition({bounds.topLeft.x, bounds.topLeft.y});
                sprite.setScale({scaleX, scaleY});
            }

            windowManager.GetWindow()->draw(sprite);

        }

        return kContinue;
    }
};

}  // namespace ECSEngine
