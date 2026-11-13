#pragma once

#include <unordered_map>

#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "CameraComponent.h"
#include "CollisionComponent.h"

namespace ECSEngine {

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();
        auto& windowManager = engine->GetWindowManager();
        auto& cameras = entityManager.template GetComponentStorage<CameraComponent>();

        for (size_t i = 0; i < cameras.mStorage.size(); i++) {
            EntityID cameraID = cameras.mOwners[i];
            if (!entityManager.IsValidEntity(cameraID)) continue;

            auto& camera = cameras.mStorage[i];

            if (camera.worldUnitsPerPixel <= 0) continue;

            // Follow the target entity along the x-axis
            if (entityManager.template HasComponent<CameraFollower>(cameraID)) {
                auto& follower = entityManager.template GetComponent<CameraFollower>(cameraID);
                EntityID targetID = follower.entityID;

                if (entityManager.template HasComponent<LocationComponent>(targetID)) {
                    auto& location = entityManager.template GetComponent<LocationComponent>(targetID);
                    float targetX = location.loc.x;

                    // Use the center of the collision box when available
                    if (entityManager.template HasComponent<CollisionComponent>(targetID)) {
                        auto& collision = entityManager.template GetComponent<CollisionComponent>(targetID);
                        targetX += collision.collisionBox.topLeft.x + collision.collisionBox.width / 2.0f;
                    }

                    float windowWidth = (float)windowManager.GetLogicalSize().x;
                    float screenX = (targetX - camera.position.x) /
                                    camera.worldUnitsPerPixel + windowWidth / 2.0f;

                    float softLeft = camera.softExclusion.topLeft.x;
                    float softRight = softLeft + camera.softExclusion.width;
                    float hardLeft = camera.hardExclusion.topLeft.x;
                    float hardRight = hardLeft + camera.hardExclusion.width;

                    // Smoothly follow when the target leaves the center area
                    float desiredX = camera.position.x;

                    if (screenX < softLeft) {
                        desiredX -= (softLeft - screenX) * camera.worldUnitsPerPixel;
                    }
                    else if (screenX > softRight) {
                        desiredX += (screenX - softRight) * camera.worldUnitsPerPixel;
                    }

                    float speed = camera.speed;
                    if (speed < 0.0f) speed = 0.0f;
                    if (speed > 1.0f) speed = 1.0f;

                    camera.position.x += (desiredX - camera.position.x) * speed;

                    // Ensure the target never passes the hard screen limits
                    screenX = (targetX - camera.position.x) /
                            camera.worldUnitsPerPixel + windowWidth / 2.0f;

                    if (screenX < hardLeft) {
                        camera.position.x = targetX -
                            (hardLeft - windowWidth / 2.0f) * camera.worldUnitsPerPixel;
                    }
                    else if (screenX > hardRight) {
                        camera.position.x = targetX -
                            (hardRight - windowWidth / 2.0f) * camera.worldUnitsPerPixel;
                    }
                }
            }

            // Apply temporary camera shake without changing the base position
            Point2D displayPosition = camera.position;

            if (entityManager.template HasComponent<CameraShake>(cameraID)) {
                auto& shake = entityManager.template GetComponent<CameraShake>(cameraID);

                if (shake.frameLimit > 0) {
                    float direction = 1.0f;
                    if (shake.frameLimit % 2 == 0) direction = -1.0f;

                    displayPosition.x += shake.offset.x * direction;
                    displayPosition.y += shake.offset.y * direction;

                    shake.frameLimit--;
                }
            }

            windowManager.SetWorldScale(camera.worldUnitsPerPixel);
            windowManager.SetWorldCenter(displayPosition);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
