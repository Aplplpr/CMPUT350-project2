//the following test is writen by ChatGPT
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "ECSEngine.h"
#include "CollisionLateSystem.h"
#include "EntityManager.h"
#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "GravitySystem.h"
#include "MovementSystem.h"
using namespace ECSEngine;

using TestManager = EntityManager<
    LocationComponent, MovementComponent, CollisionComponent,
    SpriteComponent, SpawnComponent, CameraComponent,
    CameraFollower, InputComponent, GravityComponent,
    CameraShake, ScoreComponent, RespawnComponent>;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename T>
void TestComponent(TestManager& manager, EntityID entity, T component,
                   const std::string& name) {
    Check(!manager.HasComponent<T>(entity), name + " initially absent");
    manager.AddComponent(entity, std::move(component));
    Check(manager.HasComponent<T>(entity), name + " added");
    (void)manager.GetComponent<T>(entity);
    manager.RemoveComponent<T>(entity);
    Check(!manager.HasComponent<T>(entity), name + " removed");
}

void TestAllComponentTypes() {
    TestManager manager;
    const EntityID entity = manager.CreateEntity("test");

    TestComponent(manager, entity, LocationComponent{Point2D(10.f, 20.f)}, "Location");
    TestComponent(manager, entity, MovementComponent{Point2D(2.f, 3.f)}, "Movement");
    TestComponent(manager, entity, CollisionComponent{}, "Collision");
    TestComponent(manager, entity, SpriteComponent{}, "Sprite");
    TestComponent(manager, entity, SpawnComponent{}, "Spawn");
    TestComponent(manager, entity, CameraComponent{}, "Camera");
    TestComponent(manager, entity, CameraFollower{entity}, "CameraFollower");
    TestComponent(manager, entity, InputComponent(entity), "Input");
    TestComponent(manager, entity, GravityComponent(Point2D(0.f, 980.f)), "Gravity");
    TestComponent(manager, entity, CameraShake{}, "CameraShake");
    TestComponent(manager, entity, ScoreComponent{}, "Score");
    TestComponent(manager, entity, RespawnComponent{Point2D(0.f, 0.f)}, "Respawn");

    Check(manager.GetComponentStorage<LocationComponent>().mStorage.empty(),
          "No leftover Location component");
}

void TestMultipleEntitiesAndHoles() {
    TestManager manager;
    const EntityID player = manager.CreateEntity("player");
    const EntityID wall = manager.CreateEntity("wall");
    const EntityID star = manager.CreateEntity("star");

    Check(player != wall && wall != star && player != star,
          "New entities must have unique IDs");

    manager.AddComponent(player, LocationComponent{Point2D(10.f, 20.f)});
    manager.AddComponent(player, MovementComponent{Point2D(1.f, 0.f)});
    manager.AddComponent(wall, LocationComponent{Point2D(40.f, 50.f)});
    manager.AddComponent(wall, CollisionComponent{});
    manager.AddComponent(star, LocationComponent{Point2D(70.f, 80.f)});
    manager.AddComponent(star, SpriteComponent{});

    // Replacement must not create another component of the same type.
    manager.AddComponent(player, LocationComponent{Point2D(11.f, 22.f)});
    Check(manager.GetComponent<LocationComponent>(player).loc.x == 11.f,
          "Replacing Location changes its value");
    Check(manager.GetComponentStorage<LocationComponent>().mStorage.size() == 3,
          "Replacing does not add duplicate Location components");

    // Remove a middle slot: the final element must be remapped correctly.
    manager.RemoveComponent<LocationComponent>(wall);
    Check(manager.GetComponent<LocationComponent>(star).loc.x == 70.f,
          "Moving final component must update its owner's index");
    Check(manager.GetComponentStorage<LocationComponent>().mStorage.size() == 2,
          "Removing a component must shrink dense storage");

    manager.RemoveEntity(wall);
    manager.RemoveEntity(wall);  // Repeated removal must not add a duplicate free ID.
    Check(!manager.IsValidEntity(wall), "Removed entity is invalid");
    Check(!manager.HasComponent<CollisionComponent>(wall),
          "Removed entity has no components");

    std::size_t live = 0;
    for (const auto& entity : manager) {
        if (manager.IsValidEntity(entity.GetID())) {
            ++live;
        }
    }
    Check(live == 2, "Iteration must skip invalid entity holes");

    const EntityID replacement = manager.CreateEntity("replacement");
    Check(replacement == wall, "Released entity ID must be reusable");
    Check(!manager.HasComponent<CollisionComponent>(replacement),
          "Reused entity ID must not inherit old components");
    Check(!manager.HasComponent<LocationComponent>(replacement),
          "Reused entity ID must start without Location");
}

void TestInvalidEntities() {
    TestManager manager;
    const EntityID entity = manager.CreateEntity("test");
    const EntityID invalid = 99999;

    Check(!manager.IsValidEntity(invalid), "Out-of-range entity is invalid");
    Check(!manager.HasComponent<LocationComponent>(invalid),
          "HasComponent returns false for invalid ID");
    Check(!manager.HasComponent<LocationComponent>(entity),
          "Missing component returns false");

    bool threw = false;
    try {
        (void)manager.GetComponent<LocationComponent>(entity);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw, "GetComponent throws for absent component");

    threw = false;
    try {
        manager.AddComponent(invalid, LocationComponent{});
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw, "AddComponent throws for invalid entity");

    manager.RemoveComponent<LocationComponent>(invalid);
    manager.RemoveEntity(invalid);
    Check(manager.IsValidEntity(entity), "Invalid removals do not damage valid entities");
}

struct TrackedComponent {
    static inline int alive = 0;
    int value;

    explicit TrackedComponent(int v) : value(v) { ++alive; }
    TrackedComponent(const TrackedComponent& other) : value(other.value) { ++alive; }
    TrackedComponent(TrackedComponent&& other) noexcept : value(other.value) { ++alive; }
    TrackedComponent& operator=(const TrackedComponent&) = default;
    TrackedComponent& operator=(TrackedComponent&&) noexcept = default;
    ~TrackedComponent() { --alive; }
};

void TestComponentLifetime() {
    using LifeManager = EntityManager<TrackedComponent>;
    {
        LifeManager manager;
        const EntityID a = manager.CreateEntity("a");
        const EntityID b = manager.CreateEntity("b");
        manager.AddComponent(a, TrackedComponent(1));
        manager.AddComponent(b, TrackedComponent(2));
        Check(TrackedComponent::alive == 2, "Two managed components alive");

        manager.RemoveComponent<TrackedComponent>(a);
        Check(TrackedComponent::alive == 1, "Removal must destroy one component");
        manager.RemoveEntity(b);
        Check(TrackedComponent::alive == 0, "Entity removal destroys its component");
    }
    Check(TrackedComponent::alive == 0, "No components survive manager destruction");
}


using LateManager = EntityManager<
    LocationComponent,
    CollisionComponent
>;

class LateTestView : public EngineView<
    LocationComponent, CollisionComponent> {
public:
    LateManager entities;

    SoundManager& GetSoundManager() override {
        throw std::logic_error("SoundManager not needed");
    }

    SpriteManager& GetSpriteManager() override {
        throw std::logic_error("SpriteManager not needed");
    }

    LateManager& GetEntityManager() override {
        return entities;
    }

    WindowManager& GetWindowManager() override {
        throw std::logic_error("WindowManager not needed");
    }
};

void TestCollisionLateSystem() {
    LateTestView view;
    auto& manager = view.entities;

    const EntityID player = manager.CreateEntity("player");

    manager.AddComponent(
        player,
        LocationComponent{Point2D(100.f, 200.f)}
    );

    CollisionComponent initial{};
    initial.collisionBox = {{5.f, 10.f}, 30.f, 40.f};
    initial.onGround = 1;
    initial.onRightWall = 1;
    initial.doesDamage = 1;

    manager.AddComponent(player, initial);

    CollisionLateSystem<
        LocationComponent,
        CollisionComponent
    > system;

    Check(system.Run(&view) == kContinue,
          "A04 must return kContinue");

    {
        const auto& collision =
            manager.GetComponent<CollisionComponent>(player);

        Check(collision.previousOnGround == 1,
              "Previous ground state not saved");

        Check(collision.previousOnRightWall == 1,
              "Previous wall state not saved");

        Check(collision.onGround == 0 &&
              collision.onRightWall == 0,
              "Current collision states not cleared");

        Check(collision.hasPreviousCollisionBox,
              "Previous collision box not initialized");

        Check(collision.previousCollisionBox.topLeft.x == 105.f &&
              collision.previousCollisionBox.topLeft.y == 210.f,
              "Incorrect world collision position");

        Check(collision.previousCollisionBox.width == 30.f &&
              collision.previousCollisionBox.height == 40.f,
              "Collision box dimensions changed");

        Check(collision.doesDamage == 1,
              "Entity attributes should not change");
    }

    // Simulate the next frame.
    manager.GetComponent<LocationComponent>(player).loc =
        Point2D(120.f, 220.f);

    manager.GetComponent<CollisionComponent>(player).onCeiling = 1;

    system.Run(&view);

    {
        const auto& collision =
            manager.GetComponent<CollisionComponent>(player);

        Check(collision.previousOnGround == 0 &&
              collision.previousOnCeiling == 1,
              "Second frame collision states incorrect");

        Check(collision.onCeiling == 0,
              "Current ceiling state not cleared");

        Check(collision.previousCollisionBox.topLeft.x == 125.f &&
              collision.previousCollisionBox.topLeft.y == 230.f,
              "Previous box not updated after movement");
    }

    // Entity with Collision but no Location.
    const EntityID orphan = manager.CreateEntity("orphan");

    CollisionComponent noLocation{};
    noLocation.hasPreviousCollisionBox = true;
    noLocation.onLeftWall = 1;

    manager.AddComponent(orphan, noLocation);

    system.Run(&view);

    {
        const auto& collision =
            manager.GetComponent<CollisionComponent>(orphan);

        Check(!collision.hasPreviousCollisionBox,
              "Missing Location should invalidate world box");

        Check(collision.previousOnLeftWall == 1 &&
              collision.onLeftWall == 0,
              "Collision flags not handled without Location");
    }

    // Entity ID reuse must not preserve old collision state.
    const EntityID dead = manager.CreateEntity("dead");
    manager.AddComponent(dead, CollisionComponent{});

    manager.RemoveEntity(dead);

    const EntityID replacement =
        manager.CreateEntity("replacement");

    Check(replacement == dead,
          "Entity ID was not reused");

    manager.AddComponent(
        replacement,
        LocationComponent{Point2D(3.f, 4.f)}
    );
    manager.AddComponent(replacement, CollisionComponent{});

    system.Run(&view);

    const auto& collision =
        manager.GetComponent<CollisionComponent>(replacement);

    Check(collision.previousOnGround == 0 &&
          collision.hasPreviousCollisionBox,
          "Invalid collision state after Entity ID reuse");
}


using GravityTestManager = EntityManager<
    LocationComponent,
    MovementComponent,
    GravityComponent,
    CollisionComponent
>;

class GravityTestView : public EngineView<
    LocationComponent,
    MovementComponent,
    GravityComponent,
    CollisionComponent
> {
public:
    GravityTestManager entities;

    SoundManager& GetSoundManager() override {
        throw std::logic_error("SoundManager not needed");
    }

    SpriteManager& GetSpriteManager() override {
        throw std::logic_error("SpriteManager not needed");
    }

    GravityTestManager& GetEntityManager() override {
        return entities;
    }

    WindowManager& GetWindowManager() override {
        throw std::logic_error("WindowManager not needed");
    }
};

void TestGravitySystem() {
    GravityTestView view;
    auto& manager = view.entities;

    GravitySystem<
        LocationComponent,
        MovementComponent,
        GravityComponent,
        CollisionComponent
    > system(0.5f);

    Check(system.Run(&view) == kContinue,
          "Empty GravitySystem must continue");

    // Test upward movement and fall multiplier
    const EntityID player = manager.CreateEntity("player");

    manager.AddComponent(
        player,
        MovementComponent{Point2D(1.f, -10.f)}
    );

    manager.AddComponent(
        player,
        GravityComponent(Point2D(4.f, 8.f), 0, 2.f)
    );

    manager.AddComponent(player, CollisionComponent{});

    system.Run(&view);

    auto& playerMovement =
        manager.GetComponent<MovementComponent>(player);

    Check(playerMovement.velocity.x == 3.f &&
          playerMovement.velocity.y == -6.f,
          "Gravity must update velocity correctly");

    playerMovement.velocity.y = 2.f;
    system.Run(&view);

    Check(playerMovement.velocity.y == 10.f,
          "Fall multiplier not applied");

    // Test grounded and jumping behavior
    const EntityID grounded = manager.CreateEntity("grounded");

    manager.AddComponent(
        grounded,
        MovementComponent{Point2D(0.f, 6.f)}
    );

    manager.AddComponent(
        grounded,
        GravityComponent(Point2D(4.f, 8.f))
    );

    CollisionComponent groundCollision{};
    groundCollision.previousOnGround = 1;
    manager.AddComponent(grounded, groundCollision);

    system.Run(&view);

    auto& groundMovement =
        manager.GetComponent<MovementComponent>(grounded);

    Check(groundMovement.velocity.x == 2.f &&
          groundMovement.velocity.y == 0.f,
          "Grounded entity should not fall");

    groundMovement.velocity.y = -10.f;
    system.Run(&view);

    Check(groundMovement.velocity.y == -6.f,
          "Jumping must still receive gravity");

    // Test static entities
    const EntityID wall = manager.CreateEntity("wall");

    manager.AddComponent(
        wall,
        MovementComponent{Point2D(0.f, 3.f)}
    );

    manager.AddComponent(
        wall,
        GravityComponent(Point2D(0.f, 8.f))
    );

    CollisionComponent staticCollision{};
    staticCollision.staticEntity = 1;
    manager.AddComponent(wall, staticCollision);

    system.Run(&view);

    Check(manager.GetComponent<MovementComponent>(wall).velocity.y == 3.f,
          "Static entity must not receive gravity");

    // Test disabledTimer
    const EntityID star = manager.CreateEntity("star");

    manager.AddComponent(
        star,
        MovementComponent{Point2D(0.f, 0.f)}
    );

    manager.AddComponent(
        star,
        GravityComponent(Point2D(0.f, 8.f), 2)
    );

    system.Run(&view);
    system.Run(&view);

    Check(manager.GetComponent<MovementComponent>(star).velocity.y == 0.f,
          "Gravity must remain disabled for two updates");

    system.Run(&view);

    Check(manager.GetComponent<MovementComponent>(star).velocity.y == 4.f,
          "Gravity must resume after disabledTimer expires");

    // Test an entity without MovementComponent
    const EntityID noMovement =
        manager.CreateEntity("noMovement");

    manager.AddComponent(
        noMovement,
        GravityComponent(Point2D(0.f, 8.f))
    );

    Check(system.Run(&view) == kContinue,
          "Missing MovementComponent must be handled");
}

void TestMovementSystem() {
    GravityTestView view;
    auto& manager = view.entities;

    MovementSystem<
        LocationComponent,
        MovementComponent,
        GravityComponent,
        CollisionComponent
    > system(0.5f);

    Check(system.Run(&view) == kContinue,
          "Empty MovementSystem must continue");

    // Dynamic entity with positive and negative velocity
    const EntityID player = manager.CreateEntity("player");

    manager.AddComponent(
        player,
        LocationComponent{Point2D(10.f, 20.f)}
    );

    manager.AddComponent(
        player,
        MovementComponent{Point2D(4.f, -6.f)}
    );

    system.Run(&view);

    auto& playerLocation =
        manager.GetComponent<LocationComponent>(player);

    Check(playerLocation.loc.x == 12.f &&
          playerLocation.loc.y == 17.f,
          "Movement must update both position coordinates");

    Check(manager.GetComponent<MovementComponent>(player).velocity.x == 4.f,
          "Movement must not change velocity");

    system.Run(&view);

    Check(playerLocation.loc.x == 14.f &&
          playerLocation.loc.y == 14.f,
          "Movement must accumulate across frames");

    // A static entity must not move
    const EntityID wall = manager.CreateEntity("wall");

    manager.AddComponent(
        wall,
        LocationComponent{Point2D(50.f, 50.f)}
    );

    manager.AddComponent(
        wall,
        MovementComponent{Point2D(3.f, 6.f)}
    );

    CollisionComponent staticCollision{};
    staticCollision.staticEntity = 1;
    manager.AddComponent(wall, staticCollision);

    system.Run(&view);

    const auto& wallLocation =
        manager.GetComponent<LocationComponent>(wall);

    Check(wallLocation.loc.x == 50.f &&
          wallLocation.loc.y == 50.f,
          "Static entity must not move");

    // Dynamic entity with CollisionComponent
    const EntityID star = manager.CreateEntity("star");

    manager.AddComponent(
        star,
        LocationComponent{Point2D(10.f, 10.f)}
    );

    manager.AddComponent(
        star,
        MovementComponent{Point2D(-4.f, 8.f)}
    );

    manager.AddComponent(star, CollisionComponent{});

    system.Run(&view);

    const auto& starLocation =
        manager.GetComponent<LocationComponent>(star);

    Check(starLocation.loc.x == 8.f &&
          starLocation.loc.y == 14.f,
          "Dynamic collision entity must move");

    // MovementComponent without LocationComponent
    const EntityID noLocation =
        manager.CreateEntity("noLocation");

    manager.AddComponent(
        noLocation,
        MovementComponent{Point2D(4.f, 6.f)}
    );

    Check(system.Run(&view) == kContinue,
          "Missing LocationComponent must be handled");

    // LocationComponent without MovementComponent
    const EntityID noMovement =
        manager.CreateEntity("noMovement");

    manager.AddComponent(
        noMovement,
        LocationComponent{Point2D(7.f, 9.f)}
    );

    system.Run(&view);

    const auto& stationary =
        manager.GetComponent<LocationComponent>(noMovement);

    Check(stationary.loc.x == 7.f &&
          stationary.loc.y == 9.f,
          "Entity without MovementComponent must remain stationary");

    // Invalid time step
    bool threw = false;

    try {
        MovementSystem<
            LocationComponent,
            MovementComponent,
            GravityComponent,
            CollisionComponent
        > invalidSystem(0.f);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "Zero delta time must be rejected");
}
int main() {
    try {
        TestAllComponentTypes();
        TestMultipleEntitiesAndHoles();
        TestInvalidEntities();
        TestComponentLifetime();

        TestCollisionLateSystem();
        TestGravitySystem();
        TestMovementSystem();

        std::cout << "A02, A04, A05 and A06 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}