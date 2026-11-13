#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "SystemManager.h"
#include <cmath>
#include <stdexcept>
namespace ECSEngine {

constexpr float kMaxRunVelocity = 280.f;
constexpr float kDashVelocity = 800.f;
constexpr float kJumpVelocity = 480.f;
constexpr float kGroundRunAcceleration = 1800.f;
constexpr float kAirRunAcceleration = 3000.f;
constexpr float kWallJumpVelocity = 360.f;
constexpr float kWallSlideVelocity = 110.f;
constexpr float kFastFallAcceleration = 1200.f;
constexpr float kJumpReleaseMultiplier = 0.45f;
constexpr int kDashInterval = 60;
constexpr int kDashDuration = 9;
constexpr int kWallJumpLock = 2;

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    explicit InputSystem(float deltaTime = 1.f / 60.f): mDeltaTime(deltaTime) {
        if (!std::isfinite(deltaTime) || deltaTime <= 0.f) {
            throw std::invalid_argument("Invalid input delta time");
        }
    }
    const std::string& GetName() const override {
        static const std::string name("InputSystem");
        return name;
    }
    
tSystemReturn Run(EngineView<Components...>* engine) override {
    auto& manager = engine->GetEntityManager();
    auto& inputs = manager.template GetComponentStorage<InputComponent>();
    for (std::size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (!manager.IsValidEntity(inputs.mOwners[i])) {
            continue;
        }
        auto& input = inputs.mStorage[i];
        input.jumpStarted = false;
        EntityID id = input.entity;
        if (!manager.IsValidEntity(id) || !manager.template HasComponent<MovementComponent>(id)) {
            continue;
        }
        CollisionComponent* collision = nullptr;
        if (manager.template HasComponent<CollisionComponent>(id)) {
            collision = &manager.template GetComponent<CollisionComponent>(id);
            if (collision->staticEntity) {
                continue;
            }
        }
        auto& movement = manager.template GetComponent<MovementComponent>(id);
        const bool grounded =collision && collision->previousOnGround;
        const bool leftWall = collision && collision->previousOnLeftWall;
        const bool rightWall = collision && collision->previousOnRightWall;
        const bool left = input.keydown.test(Key::Left);
        const bool right = input.keydown.test(Key::Right);
        const bool down = input.keydown.test(Key::Down);
        const bool z = input.keydown.test(Key::Z);
        const bool oldZ = input.lastKeydown.test(Key::Z);
        const bool c = input.keydown.test(Key::C);
        const bool oldC = input.lastKeydown.test(Key::C);
        int direction =static_cast<int>(right) - static_cast<int>(left);
        if (direction != 0) {
            input.facing = direction;
        }
        if (input.lastDash > 0) {
            --input.lastDash;
        }
        const bool dashing = input.dashFramesRemaining > 0;
        if (dashing) {
            --input.dashFramesRemaining;
        }
        const bool wallJumpLocked = input.wallJumpLockFrames > 0;
        if (wallJumpLocked) {
            --input.wallJumpLockFrames;
        }
        // Normal horizontal movement.
        if (!dashing && !wallJumpLocked) {
            float acceleration = grounded ? kGroundRunAcceleration: kAirRunAcceleration;
            float target = static_cast<float>(direction) * kMaxRunVelocity;
            float change = acceleration * mDeltaTime;
            if (movement.velocity.x < target) {
                movement.velocity.x = std::min(movement.velocity.x + change, target);
            } 
            else if (movement.velocity.x > target) {
                movement.velocity.x =std::max(movement.velocity.x - change, target);
            }
        }
        // Wall slide.
        bool towardWall =(left && leftWall) || (right && rightWall);
        if (!grounded && towardWall && movement.velocity.y > kWallSlideVelocity) { 
            movement.velocity.y = kWallSlideVelocity;
        }
        // Fast fall.
        if (down && movement.velocity.y >= 0.f) {
            movement.velocity.y += kFastFallAcceleration * mDeltaTime;
        }
        // Variable jump height.
        if (!z && oldZ && movement.velocity.y < 0.f) {
            movement.velocity.y *= kJumpReleaseMultiplier;
        }
        // Ground jump or wall jump.
        if (z && !oldZ) {
            if (grounded) {
                movement.velocity.y = -kJumpVelocity;
                input.jumpStarted = true;
            } 
            else if (movement.velocity.y >= 0.f &&(leftWall || rightWall)) {
                int away = leftWall ? 1 : -1;
                movement.velocity.x =static_cast<float>(away) * kWallJumpVelocity;
                movement.velocity.y = -kJumpVelocity;
                input.wallJumpLockFrames = kWallJumpLock;
                input.facing = away;
                input.jumpStarted = true;
            }
        }
        // Dash.
        if (c && !oldC && input.lastDash == 0) {
            movement.velocity.x = static_cast<float>(input.facing) * kDashVelocity;
            input.lastDash = kDashInterval;
            input.dashFramesRemaining = kDashDuration;
        }
    }
    return kContinue;
}
private:
    float mDeltaTime;
};

}  // namespace ECSEngine
