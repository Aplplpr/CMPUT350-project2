#pragma once

#include <cstdlib>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    
tSystemReturn Run(EngineView<Components...>* engine) override {
    auto& entityManager = engine->GetEntityManager();
    auto& spawners = entityManager.template GetComponentStorage<SpawnComponent>();

    for (size_t i = 0; i < spawners.mStorage.size(); i++) {
        auto& spawner = spawners.mStorage[i];

        if (!entityManager.IsValidEntity(spawner.entityID)) continue;
        if (!entityManager.template HasComponent<LocationComponent>(spawner.entityID)) continue;
        if (spawner.totalSpawns == 0) continue;

        // Update spawn timer
        spawner.timeToNextSpawn--;

        if (spawner.timeToNextSpawn > 0) continue;

        // Reset timer and update remaining spawn count
        spawner.timeToNextSpawn = spawner.timeBetweenSpawns;
        spawner.totalSpawns--;

        // Update remaining spawn count for finite spawners
        if (spawner.totalSpawns > 0) {
            spawner.totalSpawns--;
        }

        if (spawner.spawnType != "Star") continue;

        auto& spawnLocation = entityManager.template GetComponent<LocationComponent>(spawner.entityID);

        // Create star entity
        EntityID starID = entityManager.CreateEntity("Star");

        LocationComponent location;
        location.loc = Point2D(spawnLocation.loc.x + 16.0f,
                               spawnLocation.loc.y + 16.0f);
        entityManager.AddComponent(starID, location);

        // Give the star a random initial velocity
        MovementComponent movement;
        movement.velocity = Point2D(
            (float)(std::rand() % 401 - 200),
            (float)(-150 - std::rand() % 151)
        );
        entityManager.AddComponent(starID, movement);

        // Apply gravity to the spawned star
        GravityComponent gravity(Point2D(0.0f, 700.0f));
        entityManager.AddComponent(starID, gravity);

        // Register star collision
        CollisionComponent collision;
        collision.kind = CollisionKind::Star;
        collision.staticEntity = false;
        collision.collisionBox = {{4.0f, 4.0f}, 24.0f, 24.0f};
        entityManager.AddComponent(starID, collision);

        // Register star sprite
        SpriteComponent sprite;
        sprite.textureID = spawner.spawnTextureID;
        sprite.bounds = {{0.0f, 0.0f}, 32.0f, 32.0f};
        sprite.worldSpace = true;
        entityManager.AddComponent(starID, sprite);
    }

    return kContinue;
}

};

}  // namespace ECSEngine
