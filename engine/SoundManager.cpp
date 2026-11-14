
#include "SoundManager.h"
#include <stdexcept>
#include <unordered_map>

namespace ECSEngine {

/**
 * @brief Loads and registers a sound under a unique name.
 *
 * @param soundPath Path to the audio file.
 * @param soundName Unique name used to identify the sound.
 *
 * Creates a sound buffer from the specified file and associates
 * it with an SFML sound object. Both resources are stored using
 * smart pointers to manage their lifetimes automatically.
 * Throws std::runtime_error if the name is already registered
 * or the audio file cannot be loaded.
 *
 * AI-generated documentation.
 */
void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    if (mSounds.find(soundName) != mSounds.end()) {
        throw std::runtime_error("Sound already registered: " + soundName);
    }

    auto buffer = std::make_unique<sf::SoundBuffer>();

    if (!buffer->loadFromFile(soundPath)) {
        throw std::runtime_error("Failed to load sound: " + soundPath);
    }

    auto sound = std::make_unique<sf::Sound>(*buffer);

    mBuffers[soundName] = std::move(buffer);
    mSounds[soundName] = std::move(sound);
}

/**
 * @brief Retrieves a previously registered sound.
 *
 * @param soundName Unique name of the requested sound.
 * @return Reference to the corresponding SFML sound object.
 *
 * Looks up the sound by name without creating or loading
 * additional resources. Throws std::runtime_error if the
 * requested sound has not been registered.
 *
 * AI-generated documentation.
 */
sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    auto it = mSounds.find(soundName);

    if (it == mSounds.end()) {
        throw std::runtime_error("Sound not found: " + soundName);
    }

    return *(it->second);
}

}  // namespace ECSEngine
