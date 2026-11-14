#pragma once

#include <memory>
#include <string>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "CollisionEvents.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "ScoreComponent.h"
#include "SystemManager.h"
#include "LocationComponent.h"
#include "SpawnComponent.h"

namespace ECSEngine {

template <typename... Components>
class CollisionEventSystem : public System<Components...> {
public:
    CollisionEventSystem(std::shared_ptr<CollisionEventQueue> events)
        : mEvents(events) {}

    const std::string& GetName() const override {
        static std::string name("CollisionEventSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();
        auto& soundManager = engine->GetSoundManager();

        // Handle jump sounds
        auto& inputs = entityManager.template GetComponentStorage<InputComponent>();

        for (size_t i = 0; i < inputs.mStorage.size(); i++) {
            if (!entityManager.IsValidEntity(inputs.mOwners[i])) continue;

            auto& input = inputs.mStorage[i];

            if (input.jumpStarted) {
                if (input.wallJumpLockFrames > 0) {
                    soundManager.GetSound("wallJump").play();
                }
                else {
                    soundManager.GetSound("jump").play();
                }
            }
            if (input.dashFramesRemaining == 9) {
                soundManager.GetSound("dash").play();
            }
        }

        if (!mEvents) return kContinue;

        // Process collision events
        for (const auto& event : mEvents->pending) {
            if (!entityManager.IsValidEntity(event.entityA) ||
                !entityManager.IsValidEntity(event.entityB)) {
                continue;
            }

            // Check that entity IDs still refer to the same objects
            if (!entityManager.template HasComponent<CollisionComponent>(event.entityA) ||
                !entityManager.template HasComponent<CollisionComponent>(event.entityB)) {
                continue;
            }

            auto& collisionA = entityManager.template GetComponent<CollisionComponent>(event.entityA);
            auto& collisionB = entityManager.template GetComponent<CollisionComponent>(event.entityB);

            if (collisionA.instanceId != event.instanceA ||
                collisionB.instanceId != event.instanceB) {
                continue;
            }

            bool playerA = event.kindA == CollisionKind::Player;
            bool playerB = event.kindB == CollisionKind::Player;
            bool starA = event.kindA == CollisionKind::Star;
            bool starB = event.kindB == CollisionKind::Star;
            bool terrainA = event.kindA == CollisionKind::Terrain;
            bool terrainB = event.kindB == CollisionKind::Terrain;

            // Spike Damage -> Reset Level
            /**
            * @brief Removes spawned stars before resetting the level.
            *
            * Collects star entity IDs before deletion because removing
            * entities may reorganize component storage and invalidate
            * references during iteration.
            *
            * AI-generated documentation.
            */
            if (event.damaging && (playerA || playerB)) {
                EntityID playerID = playerA ? event.entityA : event.entityB;

                if (!entityManager.template HasComponent<RespawnComponent>(playerID) ||
                    !entityManager.template HasComponent<LocationComponent>(playerID)) {
                    continue;
                }

                auto& respawn =
                    entityManager.template GetComponent<RespawnComponent>(playerID);
                Point2D startPosition = respawn.loc;

                // Remove all uncollected stars
                auto& collisions =
                    entityManager.template GetComponentStorage<CollisionComponent>();

                std::vector<EntityID> starsToRemove;

                for (size_t i = 0; i < collisions.mStorage.size(); i++) {
                    if (collisions.mStorage[i].kind == CollisionKind::Star &&
                        entityManager.IsValidEntity(collisions.mOwners[i])) {
                        starsToRemove.push_back(collisions.mOwners[i]);
                    }
                }

                for (EntityID starID : starsToRemove) {
                    entityManager.RemoveEntity(starID);
                }

                // Restart star spawning
                auto& spawners =
                    entityManager.template GetComponentStorage<SpawnComponent>();

                for (auto& spawner : spawners.mStorage) {
                    spawner.timeToNextSpawn = spawner.timeBetweenSpawns;
                    spawner.totalSpawns = -1;
                }

                // Reset score
                auto& scores =
                    entityManager.template GetComponentStorage<ScoreComponent>();

                for (auto& score : scores.mStorage) {
                    score.score = 0;
                }

                // Reset player position and movement
                auto& location =
                    entityManager.template GetComponent<LocationComponent>(playerID);
                location.loc = startPosition;

                if (entityManager.template HasComponent<MovementComponent>(playerID)) {
                    auto& movement =
                        entityManager.template GetComponent<MovementComponent>(playerID);
                    movement.velocity = Point2D(0.0f, 0.0f);
                }

                // Reset collision state
                auto& collision =
                    entityManager.template GetComponent<CollisionComponent>(playerID);

                collision.onGround = 0;
                collision.onCeiling = 0;
                collision.onLeftWall = 0;
                collision.onRightWall = 0;
                collision.previousOnGround = 0;
                collision.previousOnCeiling = 0;
                collision.previousOnLeftWall = 0;
                collision.previousOnRightWall = 0;
                collision.hasPreviousCollisionBox = false;

                // Reset player input state
                if (entityManager.template HasComponent<InputComponent>(playerID)) {
                    auto& input =
                        entityManager.template GetComponent<InputComponent>(playerID);

                    input.lastDash = 0;
                    input.dashFramesRemaining = 0;
                    input.wallJumpLockFrames = 0;
                    input.jumpStarted = false;
                    input.keydown.reset();
                    input.lastKeydown.reset();
                }

                // Reset camera position and shake
                auto& cameras =
                    entityManager.template GetComponentStorage<CameraComponent>();

                for (size_t i = 0; i < cameras.mStorage.size(); i++) {
                    EntityID cameraID = cameras.mOwners[i];

                    if (!entityManager.IsValidEntity(cameraID)) continue;

                    cameras.mStorage[i].position =
                        Point2D(startPosition.x + 32.0f, 384.0f);

                    if (entityManager.template HasComponent<CameraShake>(cameraID)) {
                        auto& shake =
                            entityManager.template GetComponent<CameraShake>(cameraID);

                        shake.frameLimit = 0;
                        shake.offset = Point2D(0.0f, 0.0f);
                    }
                }

                // Discard events involving the previous level state
                mEvents->Clear();
                return kContinue;
            }



            // Player collects a star
            if ((playerA && starB) || (starA && playerB)) {
                if (event.phase != ContactPhase::Begin) continue;

                EntityID starID = starA ? event.entityA : event.entityB;

                auto& scores = entityManager.template GetComponentStorage<ScoreComponent>();
                for (size_t i = 0; i < scores.mStorage.size(); i++) {
                    if (!entityManager.IsValidEntity(scores.mOwners[i])) continue;
                    scores.mStorage[i].score++;
                }

                soundManager.GetSound("collect").play();
                entityManager.RemoveEntity(starID);
                continue;
            }

            // Player collides with terrain
            if (!((playerA && terrainB) || (terrainA && playerB))) {
                continue;
            }

            if (event.phase != ContactPhase::Begin) continue;

            ContactSide side = event.sideForA;

            // Convert sideForA into the player's perspective
            if (playerB) {
                if (side == ContactSide::Ground) side = ContactSide::Ceiling;
                else if (side == ContactSide::Ceiling) side = ContactSide::Ground;
                else if (side == ContactSide::LeftWall) side = ContactSide::RightWall;
                else if (side == ContactSide::RightWall) side = ContactSide::LeftWall;
            }

            if (side == ContactSide::Ground) {
                soundManager.GetSound("land").play();
            }
            else if (side == ContactSide::LeftWall ||
                     side == ContactSide::RightWall) {
                soundManager.GetSound("wallSlide").play();
            }

            // Add camera shake based on collision strength
            if (event.impactSpeed > 50.0f) {
                auto& cameras = entityManager.template GetComponentStorage<CameraShake>();

                for (size_t i = 0; i < cameras.mStorage.size(); i++) {
                    if (!entityManager.IsValidEntity(cameras.mOwners[i])) continue;

                    auto& shake = cameras.mStorage[i];

                    float strength = event.impactSpeed / 70.0f;
                    if (strength > 12.0f) strength = 12.0f;

                    if (side == ContactSide::LeftWall ||
                        side == ContactSide::RightWall) {
                        shake.offset = Point2D(strength, 0.0f);
                    }
                    else {
                        shake.offset = Point2D(0.0f, strength);
                    }

                    shake.frameLimit = 6;
                }
            }
        }

        mEvents->Clear();
        return kContinue;
    }

private:
    std::shared_ptr<CollisionEventQueue> mEvents;
};

}  // namespace ECSEngine
