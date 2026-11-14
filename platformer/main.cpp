#include <fstream>
#include <sstream>
#include <unordered_map>
#include <memory>

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
#include "CollisionEvents.h"
#include "CollisionEventSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

struct TileInfo {
    std::string texturePath;
    ECSEngine::Rect sourceRect;
    ECSEngine::Rect collisionRect;
};

void LoadMap(Engine& engine, const std::string& path, const std::string& file, ECSEngine::SpriteID starSpriteID) {
    std::ifstream mapFile(path + file);

    if (!mapFile.is_open()) {
        std::cerr << "Failed to open map: " << path + file << "\n";
        return;
    }

    std::string word;
    int dictionaryType;

    if (!(mapFile >> word >> dictionaryType) || word != "dictionary" || (dictionaryType != 1 && dictionaryType != 2)){
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
                collision.kind = ECSEngine::CollisionKind::Terrain;
                collision.staticEntity = true;
                collision.doesDamage = (symbol == 'x');
                collision.collisionBox = tile.collisionRect;
                entityManager.AddComponent(id, collision);
            }

            if (symbol == 'S' && dictionaryType == 2) {
                ECSEngine::SpawnComponent spawner;
                spawner.entityID = id;
                spawner.spawnType = "Star";
                spawner.spawnTextureID = starSpriteID;
                spawner.timeToNextSpawn = 180;
                spawner.timeBetweenSpawns = 180;
                spawner.totalSpawns = -1; //infinite spawning

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
    auto& soundManager = engine.GetSoundManager();

    soundManager.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    soundManager.RegisterSound(gResourcePath + "sfx_jump-high.ogg", "wallJump");
    soundManager.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
    soundManager.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wallSlide");
    soundManager.RegisterSound(gResourcePath + "sfx_gem.ogg", "collect");
    soundManager.RegisterSound(gResourcePath + "laserLarge_000.ogg", "dash");

    // Register star sprite
    ECSEngine::SpriteID starSpriteID = engine.GetSpriteManager().RegisterTexture(
        gResourcePath + "spritesheet-tiles-default.png",
        {{640.0f, 320.0f}, 64.0f, 64.0f}
    );

    // Load maps
    LoadMap(engine, gResourcePath, "sky.map", starSpriteID);
    LoadMap(engine, gResourcePath, "world.map", starSpriteID);

    // Place player
    auto& entityManager = engine.GetEntityManager();
    auto& spriteManager = engine.GetSpriteManager();

    ECSEngine::EntityID playerID = entityManager.CreateEntity("Player");

    ECSEngine::LocationComponent playerLocation;
    playerLocation.loc = ECSEngine::Point2D(800.0f, 500.0f);
    entityManager.AddComponent(playerID, playerLocation);

    ECSEngine::MovementComponent playerMovement;
    playerMovement.velocity = ECSEngine::Point2D(0.0f, 0.0f);
    entityManager.AddComponent(playerID, playerMovement);

    ECSEngine::GravityComponent playerGravity(
        ECSEngine::Point2D(0.0f, 1100.0f), 0, 1.2f
    );
    entityManager.AddComponent(playerID, playerGravity);

    ECSEngine::CollisionComponent playerCollision;
    playerCollision.kind = ECSEngine::CollisionKind::Player;
    playerCollision.staticEntity = false;
    playerCollision.collisionBox = {{8.0f, 8.0f}, 48.0f, 56.0f};
    entityManager.AddComponent(playerID, playerCollision);

    // Player Sprites
    ECSEngine::SpriteComponent playerSprite;

    std::string characterSheet =
        gResourcePath + "spritesheet-characters-default.png";

    // Register animation sprites
    playerSprite.idleSpriteID = spriteManager.RegisterTexture(
        characterSheet, {{0.0f, 0.0f}, 128.0f, 128.0f}
    );

    playerSprite.walkSpriteID1 = spriteManager.RegisterTexture(
        characterSheet, {{0.0f, 128.0f}, 128.0f, 128.0f}
    );

    playerSprite.walkSpriteID2 = spriteManager.RegisterTexture(
        characterSheet, {{0.0f, 384.0f}, 128.0f, 128.0f}
    );

    playerSprite.jumpSpriteID = spriteManager.RegisterTexture(
        characterSheet, {{0.0f, 256.0f}, 128.0f, 128.0f}
    );

    // Initialize player sprite
    playerSprite.textureID = playerSprite.idleSpriteID;
    playerSprite.bounds = {{0.0f, 0.0f}, 64.0f, 64.0f};
    playerSprite.worldSpace = true;
    playerSprite.animated = true;

    entityManager.AddComponent(playerID, playerSprite);


    ECSEngine::InputComponent playerInput(playerID);
    entityManager.AddComponent(playerID, playerInput);

    ECSEngine::RespawnComponent playerRespawn;
    playerRespawn.loc = playerLocation.loc;
    entityManager.AddComponent(playerID, playerRespawn);

    // Setup score component
    ECSEngine::EntityID scoreID = entityManager.CreateEntity("Score");

    ECSEngine::ScoreComponent score;
    score.score = 0;

    // Register digit sprites 0-9
    for (int i = 0; i < 10; i++) {
        ECSEngine::Rect sourceRect = {
            {832.0f, 832.0f - i * 64.0f},
            64.0f, 64.0f
        };

        score.digitTextureID[i] = spriteManager.RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            sourceRect
        );
    }

    // Create three entities for displaying the score
    for (int i = 0; i < 3; i++) {
        ECSEngine::EntityID digitID = entityManager.CreateEntity("ScoreDigit");
        score.digitID[i] = digitID;

        ECSEngine::SpriteComponent digitSprite;
        digitSprite.textureID = score.digitTextureID[0];
        digitSprite.bounds = {
            {20.0f + i * 40.0f, 20.0f},
            40.0f, 40.0f
        };
        digitSprite.worldSpace = false;

        entityManager.AddComponent(digitID, digitSprite);
    }

    entityManager.AddComponent(scoreID, score);

    // Setup Camera
    ECSEngine::EntityID cameraID = entityManager.CreateEntity("Camera");

    ECSEngine::CameraComponent camera;
    camera.position = ECSEngine::Point2D(
        playerLocation.loc.x + 32.0f, 384.0f
    );
    camera.worldUnitsPerPixel = 1.0f;
    camera.hardExclusion = {{102.4f, 0.0f}, 819.2f, 768.0f};
    camera.softExclusion = {{307.2f, 0.0f}, 409.6f, 768.0f};
    camera.speed = 0.1f;
    entityManager.AddComponent(cameraID, camera);

    ECSEngine::CameraFollower follower;
    follower.entityID = playerID;
    entityManager.AddComponent(cameraID, follower);

    ECSEngine::CameraShake shake;
    shake.frameLimit = 0;
    shake.offset = ECSEngine::Point2D(0.0f, 0.0f);
    entityManager.AddComponent(cameraID, shake);

    engine.GetWindowManager().SetWorldScale(camera.worldUnitsPerPixel);
    engine.GetWindowManager().SetWorldCenter(camera.position);

    // Add all systems
    auto collisionEvents = std::make_shared<ECSEngine::CollisionEventQueue>();
    
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>(collisionEvents));
    sm.AddSystem(std::make_unique<ECSEngine::CollisionEventSystem<COMPONENTS>>(collisionEvents));
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
