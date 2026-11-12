
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
class GravitySystem : public System<Components...> {
public:
	explicit GravitySystem(float deltaTime = 1.f / 60.f): mDeltaTime(deltaTime) {
		if (!std::isfinite(deltaTime) || deltaTime <= 0.f) {
			throw std::invalid_argument("Delta time must be positive and finite");
		}
	}
	const std::string& GetName() const override {
		static const std::string name("GravitySystem");
		return name;
	}
	tSystemReturn Run(EngineView<Components...>* engine) override {
		auto& manager = engine->GetEntityManager();
		auto& gravities = manager.template GetComponentStorage<GravityComponent>();
		for (std::size_t i = 0; i < gravities.mStorage.size(); ++i) {
			EntityID id = gravities.mOwners[i];
			if (!manager.IsValidEntity(id) ||
				!manager.template HasComponent<MovementComponent>(id)) {
				continue;
			}
			if (manager.template HasComponent<CollisionComponent>(id) &&
				manager.template GetComponent<CollisionComponent>(id).staticEntity) {
					continue;
			}
			auto& gravity = gravities.mStorage[i];
			if (gravity.disabledTimer > 0) {
					--gravity.disabledTimer;
					continue;
			}
			auto& movement =manager.template GetComponent<MovementComponent>(id);
			movement.velocity.x += gravity.acceleration.x * mDeltaTime;
			bool grounded =manager.template HasComponent<CollisionComponent>(id) &&manager.template GetComponent<CollisionComponent>(id).previousOnGround;
			if (grounded && movement.velocity.y >= 0.f &&
				gravity.acceleration.y >= 0.f) {
				movement.velocity.y = 0.f;
				continue;
			}

			float fallMultiplier = 1.f;

			if (movement.velocity.y > 0.f && gravity.acceleration.y > 0.f) {
				fallMultiplier = gravity.fallMultiplier;
			}
			movement.velocity.y +=gravity.acceleration.y * fallMultiplier * mDeltaTime;
		}
		return kContinue;
	}
private:
  float mDeltaTime;
};

}  // namespace ECSEngine


