
#pragma once

#include <unordered_map>

#include <SFML/Audio.hpp>

#include <memory>
#include <string>

namespace ECSEngine {

/**
 * @brief Manages sound resources used by the game engine.
 *
 * Stores sound buffers and sound objects under unique names,
 * allowing registered sounds to be reused throughout the game.
 * Sound buffers are kept alive for as long as their sounds exist.
 *
 * AI-generated documentation.
 */
class SoundManager {
public:
    /**
     * @brief Creates an empty sound manager.
     *
     * AI-generated documentation.
     */
    SoundManager() = default;

    /**
     * @brief Destroys the sound manager and its stored resources.
     *
     * AI-generated documentation.
     */
    ~SoundManager() = default;

    void RegisterSound(const std::string& soundPath, const std::string& soundName);
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> mBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> mSounds;
};

}  // namespace ECSEngine
