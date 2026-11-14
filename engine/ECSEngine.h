
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

/**
 * @brief Provides systems with access to the engine's managers.
 *
 * Defines a common interface for accessing sound, sprite, entity,
 * and window resources without requiring systems to use the
 * concrete ECSEngine class directly.
 *
 * AI-generated documentation.
 */
template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

/**
 * @brief Manages the main ECS engine and its execution loop.
 *
 * Owns the engine's resource and system managers, processes
 * window and keyboard events, updates registered systems,
 * and provides optional collision debugging visualization.
 *
 * AI-generated documentation.
 */
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    /**
     * @brief Creates and initializes the ECS engine.
     *
     * @param width Initial game window width in pixels.
     * @param height Initial game window height in pixels.
     * @param name Title displayed on the game window.
     *
     * Initializes the window manager and sets the initial
     * execution and debugging states.
     *
     * AI-generated documentation.
     */
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);

    /**
     * @brief Destroys the ECS engine.
     *
     * AI-generated documentation.
     */
    ~ECSEngine();

    /**
     * @brief Runs the main game loop until the window closes.
     *
     * AI-generated documentation.
     */
    void Run();

    /**
     * @brief Returns the engine's sound manager.
     * @return Reference to the SoundManager.
     *
     * AI-generated documentation.
     */
    SoundManager& GetSoundManager() override { return mSoundManager; }

    /**
     * @brief Returns the engine's sprite manager.
     * @return Reference to the SpriteManager.
     *
     * AI-generated documentation.
     */
    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    /**
     * @brief Returns the engine's entity manager.
     * @return Reference to the EntityManager.
     *
     * AI-generated documentation.
     */
    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    /**
     * @brief Returns the engine's window manager.
     * @return Reference to the WindowManager.
     *
     * AI-generated documentation.
     */
    WindowManager& GetWindowManager() override { return mWindowManager; }

    /**
     * @brief Returns the engine's system manager.
     * @return Reference to the SystemManager.
     *
     * AI-generated documentation.
     */
    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    /**
     * @brief Processes pending window and keyboard events.
     *
     * @return True if the game should continue, or false if the
     * window has been closed.
     *
     * AI-generated documentation.
     */
    bool ProcessEvents();

    /**
     * @brief Draws collision boundaries when debugging is enabled.
     *
     * AI-generated documentation.
     */
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

/**
 * @brief Initializes the engine and its debugging state.
 *
 * @param width Initial window width in pixels.
 * @param height Initial window height in pixels.
 * @param name Title displayed on the game window.
 *
 * Constructs the WindowManager and initializes the execution
 * flags, including the disabled debug visualization state.
 *
 * AI-generated documentation.
 */
template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name),
      running(true),
      step(false),
      debug(0) {}

/**
 * @brief Destroys the engine.
 *
 * Engine resources are released through their member objects
 * when the engine is destroyed.
 *
 * AI-generated documentation.
 */
template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

/**
 * @brief Executes the engine's main game loop.
 *
 * Limits rendering to 60 FPS to keep the frame-based movement
 * and spawning systems running at their intended speed.
 * Each iteration processes input events, clears the window,
 * runs registered ECS systems, draws optional debugging
 * information, and displays the completed frame.
 *
 * The loop terminates when the window closes or a system
 * requests that the engine stop.
 *
 * AI-generated documentation.
 */
template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    window->setFramerateLimit(60);
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

/**
 * @brief Processes SFML events and updates keyboard input states.
 *
 * @return True while the game should continue running, or false
 * when a window-close event is received.
 *
 * Saves the previous keyboard state before processing new events,
 * allowing other systems to detect newly pressed or released keys.
 * Handles window closing, viewport resizing, key presses and
 * releases, and loss of window focus.
 *
 * Releasing F3 toggles collision debugging. Losing focus clears
 * the current keyboard state to prevent keys from remaining
 * pressed after switching away from the window.
 *
 * AI-generated documentation.
 */
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

        if (event->is<sf::Event::Resized>()) {
            mWindowManager.UpdateViewport();
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

/**
 * @brief Renders collision boxes for debugging.
 *
 * Iterates over entities with collision components and retrieves
 * their world positions to calculate the final collision bounds.
 * Static entities are outlined in red, while dynamic entities
 * are outlined in green.
 *
 * Debug rendering is skipped when the debug mode is disabled.
 * The visualization is used to inspect collision boundaries
 * without affecting the actual collision calculations.
 *
 * AI-generated documentation.
 */
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
