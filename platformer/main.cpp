#include <fstream>
#include <sstream>
#include <unordered_map>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream mapFile(path + file);

    if (!mapFile.is_open()) {
        std::cerr << "Failed to open map: " << path + file << "\n";
        return;
    }

    std::string word;
    int dictionaryType;

    mapFile >> word >> dictionaryType;

    if (word != "dictionary" || (dictionaryType != 1 && dictionaryType != 2)) {
        std::cerr << "Invalid map dictionary\n";
        return;
    }

    std::unordered_map<char, TileInfo> dictionary;

    while (mapFile >> word) {
        if (word == "map") break;

        char symbol = word[0];
        TileInfo tile;

        mapFile >> tile.texturePath;
        mapFile >> tile.sourceRect.topLeft.x >> tile.sourceRect.topLeft.y;
        mapFile >> tile.sourceRect.width >> tile.sourceRect.height;

        if (dictionaryType == 2) {
            mapFile >> tile.collisionRect.topLeft.x >> tile.collisionRect.topLeft.y;
            mapFile >> tile.collisionRect.width >> tile.collisionRect.height;
        }

        dictionary[symbol] = tile;
    }

    // Read map origin, tile dimensions, and grid size
    float originX, originY;
    float tileWidth, tileHeight;
    int gridWidth, gridHeight;
    std::string label;

    mapFile >> label >> originX >> originY;
    if (label != "origin") {
        std::cerr << "Invalid map origin\n";
        return;
    }

    mapFile >> label >> tileWidth >> tileHeight;
    if (label != "tile") {
        std::cerr << "Invalid tile size\n";
        return;
    }

    mapFile >> label >> gridWidth >> gridHeight;
    if (label != "size") {
        std::cerr << "Invalid map size\n";
        return;
    }

    if (!mapFile || gridWidth <= 0 || gridHeight <= 0 ||
        tileWidth <= 0 || tileHeight <= 0) {
        std::cerr << "Invalid map dimensions\n";
        return;
    }

    auto& entityManager = engine.GetEntityManager();
    auto& spriteManager = engine.GetSpriteManager();

    // Read and create each tile
    for (int row = 0; row < gridHeight; row++) {
        std::string line;
        if (!(mapFile >> line) || line.size() != (size_t)gridWidth) {
            std::cerr << "Invalid map row: " << row << "\n";
            return;
        }

        for (int col = 0; col < gridWidth; col++) {
            char symbol = line[col];

            if (symbol == '.') continue;

            auto it = dictionary.find(symbol);
            if (it == dictionary.end()) {
                std::cerr << "Unknown tile: " << symbol << "\n";
                return;
            }

            TileInfo& tile = it->second;

            ECSEngine::EntityID id = entityManager.CreateEntity("Tile");

            ECSEngine::LocationComponent location;
            location.loc = ECSEngine::Point2D(
                originX + col * tileWidth,
                originY + row * tileHeight
            );
            entityManager.AddComponent(id, location);

            ECSEngine::SpriteComponent sprite;
            sprite.textureID = spriteManager.RegisterTexture(
                path + tile.texturePath, tile.sourceRect
            );
            sprite.bounds = {{0, 0}, tileWidth, tileHeight};
            sprite.worldSpace = true;
            entityManager.AddComponent(id, sprite);

            if (dictionaryType == 2) {
                ECSEngine::CollisionComponent collision;
                collision.staticEntity = true;
                collision.collisionBox = tile.collisionRect;
                entityManager.AddComponent(id, collision);
            }

            if (symbol == 'S' && dictionaryType == 2) {
                ECSEngine::SpawnComponent spawner;
                spawner.entityID = id;
                spawner.spawnType = "Star";
                spawner.spawnTextureID = sprite.textureID; // Temporary, replace in B08
                spawner.timeToNextSpawn = 60;
                spawner.timeBetweenSpawns = 60;
                spawner.totalSpawns = 0; // Disabled until B08 configures the star sprite

                entityManager.AddComponent(id, spawner);
            }

        }
    }
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds

    // Load maps

    // Place player

    // Setup score component

    // Setup Camera

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
