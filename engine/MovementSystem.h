#pragma once
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    explicit MovementSystem(float deltaTime = 1.f / 60.f): mDeltaTime(deltaTime) {
        if (!std::isfinite(deltaTime) || deltaTime <= 0.f) {
            throw std::invalid_argument("Delta time must be positive and finite");
        }
    }

    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& manager = engine->GetEntityManager();
        auto& movements =manager.template GetComponentStorage<MovementComponent>();
        for (std::size_t i = 0; i < movements.mStorage.size(); ++i) {
            EntityID id = movements.mOwners[i];
            if (!manager.IsValidEntity(id) ||!manager.template HasComponent<LocationComponent>(id)) {
                continue;
            }
            if (manager.template HasComponent<CollisionComponent>(id) &&manager.template GetComponent<CollisionComponent>(id).staticEntity) {
                continue;
            }
            const auto& movement = movements.mStorage[i];
            auto& location = manager.template GetComponent<LocationComponent>(id);
            location.loc.x += movement.velocity.x * mDeltaTime;
            location.loc.y += movement.velocity.y * mDeltaTime;
        }
        return kContinue; 
    }
private:
    float mDeltaTime;
};

}  // namespace ECSEngine
