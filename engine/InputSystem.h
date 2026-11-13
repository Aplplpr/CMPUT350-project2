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
namespace ECSEngine {
constexpr float kMaxRunVelocity = 15.f;
constexpr float kDashVelocity = 50.f;
constexpr float kJumpVelocity = 25.f;
constexpr float kGroundRunAcceleration = 5.f;
constexpr float kAirRunAcceleration = 4.f;
constexpr int kDashInterval = 60;
template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("InputSystem");
        return name;
    }
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& manager = engine->GetEntityManager();
        auto& inputs =manager.template GetComponentStorage<InputComponent>();
        for (std::size_t i = 0; i < inputs.mStorage.size(); ++i) {
            if (!manager.IsValidEntity(inputs.mOwners[i])) {
                continue;
            }
            auto& input = inputs.mStorage[i];
            input.jumpStarted = false;
            EntityID id = input.entity;
            if (!manager.IsValidEntity(id) ||
                !manager.template HasComponent<MovementComponent>(id)) {
                continue;
            }
            bool grounded = false;
            if (manager.template HasComponent<CollisionComponent>(id)) {
                const auto& collision =manager.template GetComponent<CollisionComponent>(id);
                if (collision.staticEntity) {
                    continue;
                }
                grounded = collision.previousOnGround != 0;
            }
            auto& movement =manager.template GetComponent<MovementComponent>(id);
            if (input.lastDash > 0) {
                --input.lastDash;
            }
            const bool left = input.keydown.test(Key::Left);
            const bool right = input.keydown.test(Key::Right);
            int direction =static_cast<int>(right) - static_cast<int>(left);
            if (direction != 0) {
                input.facing = direction;
            }
            const float acceleration = grounded ? kGroundRunAcceleration : kAirRunAcceleration;
            const float target =static_cast<float>(direction) * kMaxRunVelocity;
            if (movement.velocity.x < target) {
                movement.velocity.x =std::min(movement.velocity.x + acceleration, target);
            } else if (movement.velocity.x > target) {
                movement.velocity.x =std::max(movement.velocity.x - acceleration, target);
            }
            const bool jumpPressed = input.keydown.test(Key::Z) && !input.lastKeydown.test(Key::Z);
            if (jumpPressed && grounded) {
                movement.velocity.y = -kJumpVelocity;
                input.jumpStarted = true;
            }
            const bool dashPressed =
                input.keydown.test(Key::C) && !input.lastKeydown.test(Key::C);
            if (dashPressed && input.lastDash == 0) {
                movement.velocity.x = static_cast<float>(input.facing) * kDashVelocity;
                input.lastDash = kDashInterval;
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
