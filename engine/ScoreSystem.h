#pragma once
#include "ScoreComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {

template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

/**
 * @brief Updates the score display using reusable HUD entities.
 *
 * @param engine Pointer to the engine view providing ECS access.
 * @return kContinue after updating the score display.
 *
 * Splits the score into hundreds, tens, and ones, then updates
 * the SpriteID of each existing HUD entity. This avoids repeatedly
 * creating and deleting entities when the score changes.
 *
 * The displayed score is limited to 000-999 without changing
 * the actual score stored in ScoreComponent.
 *
 * AI-generated documentation.
 */
tSystemReturn Run(EngineView<Components...>* engine) override {
    auto& entityManager = engine->GetEntityManager();
    auto& scores = entityManager.template GetComponentStorage<ScoreComponent>();

    for (size_t i = 0; i < scores.mStorage.size(); i++) {
        EntityID scoreID = scores.mOwners[i];

        if (!entityManager.IsValidEntity(scoreID)) continue;

        auto& score = scores.mStorage[i];

        int value = score.score;
        if (value < 0) value = 0;
        if (value > 999) value = 999;

        int digits[3];
        digits[0] = value / 100;
        digits[1] = (value / 10) % 10;
        digits[2] = value % 10;

        for (int j = 0; j < 3; j++) {
            EntityID digitID = score.digitID[j];

            if (!entityManager.template HasComponent<SpriteComponent>(digitID)) continue;

            auto& sprite = entityManager.template GetComponent<SpriteComponent>(digitID);
            sprite.textureID = score.digitTextureID[digits[j]];
        }
    }

    return kContinue;
}

};

}  // namespace ECSEngine
