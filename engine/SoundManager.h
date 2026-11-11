#pragma once

#include <unordered_map>

#include <SFML/Audio.hpp>

#include <memory>
#include <string>

namespace ECSEngine {

class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;
    void RegisterSound(const std::string& soundPath, const std::string& soundName);
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> mBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> mSounds;
};

}  // namespace ECSEngine
