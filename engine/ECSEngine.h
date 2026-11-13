#pragma once

#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    // For debugging - pause and step the engine
    bool running, step;
    int debug;
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name),
      running(true),
      step(false),
      debug(0) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();

    while (window->isOpen()) {
        if (!ProcessEvents()) {
            break;
        }

        window->clear();

        if (!mSystemManager.Run(this)) {
            window->close();
            break;
        }

        DebugDraw();
        window->display();
    }
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();
    auto& inputs = mEntityManager.template GetComponentStorage<InputComponent>().mStorage;

    // Save the previous frame's keyboard state
    for (auto& input : inputs) {
        input.lastKeydown = input.keydown;
    }

    // Process all pending window events
    while (const auto event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window->close();
            return false;
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            int code = static_cast<int>(key->scancode);

            if (code >= 0 && code < static_cast<int>(sf::Keyboard::ScancodeCount)) {
                for (auto& input : inputs) {
                    input.keydown.set(code, true);
                }
            }
        }

        if (const auto* key = event->getIf<sf::Event::KeyReleased>()) {
            int code = static_cast<int>(key->scancode);

            if (key->scancode == sf::Keyboard::Scancode::F3) {
                debug = 1 - debug;
            }

            if (code >= 0 && code < static_cast<int>(sf::Keyboard::ScancodeCount)) {
                for (auto& input : inputs) {
                    input.keydown.set(code, false);
                }
            }
        }

        if (event->is<sf::Event::FocusLost>()) {
            for (auto& input : inputs) {
                input.keydown.reset();
            }
        }
    }

    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    if (debug == 0) return;

    auto& collisions = mEntityManager.template GetComponentStorage<CollisionComponent>();

    for (size_t i = 0; i < collisions.mStorage.size(); i++) {
        EntityID id = collisions.mOwners[i];

        if (!mEntityManager.IsValidEntity(id)) continue;
        if (!mEntityManager.template HasComponent<LocationComponent>(id)) continue;

        auto& collision = collisions.mStorage[i];
        auto& location = mEntityManager.template GetComponent<LocationComponent>(id);

        Rect bounds = collision.collisionBox;
        bounds.topLeft.x += location.loc.x;
        bounds.topLeft.y += location.loc.y;

        if (collision.staticEntity) {
            mWindowManager.DebugFrameRect(bounds, 255, 0, 0);
        }
        else {
            mWindowManager.DebugFrameRect(bounds, 0, 255, 0);
        }
    }
}

}  // namespace ECSEngine