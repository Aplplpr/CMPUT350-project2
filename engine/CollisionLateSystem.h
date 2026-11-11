#pragma once
#include <cstddef>
#include <memory>
#include <iostream>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"
namespace ECSEngine {

template <typename... Components>
    class CollisionLateSystem : public System<Components...> {
    public:
        const std::string& GetName() const override {
            static std::string name("CollisionLateSystem");
            return name;
        }

        tSystemReturn Run(EngineView<Components...>* engine) override { 
            auto& manager = engine->GetEntityManager();
            auto& collisions = manager.template GetComponentStorage<CollisionComponent>();
            for (std::size_t i = 0; i < collisions.mStorage.size(); ++i) {
                EntityID id = collisions.mOwners[i];
                if (!manager.IsValidEntity(id)) {
                    continue;
                }
                auto& collision = collisions.mStorage[i];
                collision.previousOnGround = collision.onGround;
                collision.previousOnCeiling = collision.onCeiling;
                collision.previousOnLeftWall = collision.onLeftWall;
                collision.previousOnRightWall = collision.onRightWall;
                collision.onGround = 0;
                collision.onCeiling = 0;
                collision.onLeftWall = 0;
                collision.onRightWall = 0;
                if (!manager.template HasComponent<LocationComponent>(id)) {
                    collision.hasPreviousCollisionBox = false;
                    continue;
                }
                const auto& location =manager.template GetComponent<LocationComponent>(id);
                collision.previousCollisionBox = collision.collisionBox;
                collision.previousCollisionBox.topLeft.x += location.loc.x;
                collision.previousCollisionBox.topLeft.y += location.loc.y;
                collision.hasPreviousCollisionBox = true;
            }
            return kContinue;
        }
    };

}  // namespace ECSEngine
