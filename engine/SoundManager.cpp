#include "SoundManager.h"
#include <stdexcept>
#include <unordered_map>

namespace ECSEngine {

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

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    auto it = mSounds.find(soundName);

    if (it == mSounds.end()) {
        throw std::runtime_error("Sound not found: " + soundName);
    }

    return *(it->second);
}

}  // namespace ECSEngine
