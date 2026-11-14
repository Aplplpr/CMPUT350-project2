//the following test is writen by ChatGPT
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ECSEngine.h"
#include "EntityManager.h"
#include "CollisionLateSystem.h"
#include "CollisionEvents.h"
#include "CollisionSystem.h"
#include "GravitySystem.h"
#include "InputSystem.h"
#include "MovementSystem.h"
#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"

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


// Test views do not open a window. They expose an EntityManager to the systems.
template <typename... Components>
class UnitTestView : public EngineView<Components...> {
public:
    EntityManager<Components...> entities;

    EntityManager<Components...>& GetEntityManager() override {
        return entities;
    }
    SoundManager& GetSoundManager() override {
        throw std::logic_error("SoundManager is not used by these tests");
    }
    SpriteManager& GetSpriteManager() override {
        throw std::logic_error("SpriteManager is not used by these tests");
    }
    WindowManager& GetWindowManager() override {
        throw std::logic_error("WindowManager is not used by these tests");
    }
};

using PhysicsView = UnitTestView<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;

using PhysicsLate = CollisionLateSystem<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;
using PhysicsGravity = GravitySystem<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;
using PhysicsMovement = MovementSystem<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;
using PhysicsInput = InputSystem<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;
using PhysicsCollision = CollisionSystem<
    LocationComponent, MovementComponent, GravityComponent,
    CollisionComponent, InputComponent>;

bool Near(float actual, float expected, float tolerance = 0.05f) {
    return std::fabs(actual - expected) <= tolerance;
}

// Add a body using the real ECS API, without introducing a testing component.
EntityID AddPhysicsBody(PhysicsView& view, const std::string& name,
                        Point2D position, Rect localBox,
                        CollisionKind kind, bool isStatic = false,
                        Point2D velocity = {}) {
    auto& manager = view.entities;
    const EntityID id = manager.CreateEntity(name);
    manager.AddComponent(id, LocationComponent{position});
    CollisionComponent collision{};
    collision.kind = kind;
    collision.staticEntity = isStatic ? 1 : 0;
    collision.collisionBox = localBox;
    manager.AddComponent(id, collision);
    if (!isStatic) {
        manager.AddComponent(id, MovementComponent{velocity});
    }
    return id;
}

void SetPreviousPosition(PhysicsView& view, EntityID id,
                         Point2D previousLocation) {
    auto& collision =
        view.entities.GetComponent<CollisionComponent>(id);
    collision.previousCollisionBox = collision.collisionBox;
    collision.previousCollisionBox.topLeft.x += previousLocation.x;
    collision.previousCollisionBox.topLeft.y += previousLocation.y;
    collision.hasPreviousCollisionBox = true;
}

void TestA04CollisionLate() {
    PhysicsView view;
    const EntityID player = AddPhysicsBody(
        view, "player", {100.f, 200.f},
        {{5.f, 10.f}, 30.f, 40.f}, CollisionKind::Player);

    auto& collider = view.entities.GetComponent<CollisionComponent>(player);
    collider.onGround = 1;
    collider.onRightWall = 1;

    PhysicsLate late;
    Check(late.Run(&view) == kContinue, "A04 should continue");
    Check(collider.previousOnGround == 1 &&
              collider.previousOnRightWall == 1,
          "A04 previous contact flags");
    Check(collider.onGround == 0 && collider.onRightWall == 0,
          "A04 current contact flags should reset");
    Check(collider.hasPreviousCollisionBox &&
              Near(collider.previousCollisionBox.topLeft.x, 105.f) &&
              Near(collider.previousCollisionBox.topLeft.y, 210.f),
          "A04 world-space previous collision bounds");

    view.entities.GetComponent<LocationComponent>(player).loc = {120.f, 220.f};
    late.Run(&view);
    Check(Near(collider.previousCollisionBox.topLeft.x, 125.f) &&
              Near(collider.previousCollisionBox.topLeft.y, 230.f),
          "A04 should update snapshot each frame");
}

void TestA05Gravity() {
    PhysicsView view;
    const EntityID player = AddPhysicsBody(
        view, "player", {0.f, 0.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
        false, {1.f, -10.f});
    view.entities.AddComponent(
        player, GravityComponent(Point2D(4.f, 8.f), 0, 2.f));

    PhysicsGravity gravity(0.5f);
    gravity.Run(&view);
    auto& velocity =
        view.entities.GetComponent<MovementComponent>(player).velocity;
    Check(Near(velocity.x, 3.f) && Near(velocity.y, -6.f),
          "A05 upward gravity acceleration");

    velocity.y = 2.f;
    gravity.Run(&view);
    Check(Near(velocity.y, 10.f), "A05 falling multiplier");

    view.entities.GetComponent<CollisionComponent>(player)
        .previousOnGround = 1;
    velocity.y = 20.f;
    gravity.Run(&view);
    Check(Near(velocity.y, 0.f), "A05 grounded gravity check");

    // Gravity disabled for two updates (measured in physics frames).
    auto& component = view.entities.GetComponent<GravityComponent>(player);
    component.disabledTimer = 2;
    view.entities.GetComponent<CollisionComponent>(player)
        .previousOnGround = 0;
    velocity.y = 0.f;
    gravity.Run(&view);
    gravity.Run(&view);
    Check(Near(velocity.y, 0.f), "A05 disabled gravity interval");
    gravity.Run(&view);
    Check(Near(velocity.y, 4.f), "A05 gravity resumes after timer");
}

void TestA06Movement() {
    PhysicsView view;
    const EntityID player = AddPhysicsBody(
        view, "player", {10.f, 20.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
        false, {4.f, -6.f});
    const EntityID wall = AddPhysicsBody(
        view, "wall", {50.f, 50.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Terrain, true);
    // A stationary static object should not move even with a MovementComponent.
    view.entities.AddComponent(wall, MovementComponent{{5.f, 5.f}});

    PhysicsMovement movement(0.5f);
    Check(movement.Run(&view) == kContinue, "A06 should continue");
    const Point2D p1 =
        view.entities.GetComponent<LocationComponent>(player).loc;
    const Point2D p2 =
        view.entities.GetComponent<LocationComponent>(wall).loc;
    Check(Near(p1.x, 12.f) && Near(p1.y, 17.f),
          "A06 position = position + velocity * deltaTime");
    Check(Near(p2.x, 50.f) && Near(p2.y, 50.f),
          "A06 static entity should not move");
    movement.Run(&view);
    Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.x, 14.f),
          "A06 movement accumulates across frames");
}

void TestA08AndA09Input() {
    PhysicsView view;
    const EntityID player = AddPhysicsBody(
        view, "player", {0.f, 0.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player);
    view.entities.AddComponent(player, InputComponent(player));
    view.entities.AddComponent(
        player, GravityComponent(Point2D(0.f, 900.f)));
    auto& input = view.entities.GetComponent<InputComponent>(player);
    auto& body = view.entities.GetComponent<CollisionComponent>(player);
    auto& movement = view.entities.GetComponent<MovementComponent>(player);

    PhysicsInput system;
    PhysicsGravity gravity;
    body.previousOnGround = 1;
    input.keydown.set(Key::Right);
    system.Run(&view);
    Check(Near(movement.velocity.x, kGroundRunAcceleration / 60.f),
          "A08 should accelerate gradually to the right");
    system.Run(&view);
    system.Run(&view);
    Check(Near(movement.velocity.x, 3.f * kGroundRunAcceleration / 60.f),
          "A08 acceleration should accumulate");

    input.keydown.reset(Key::Right);
    input.keydown.set(Key::Z);
    system.Run(&view);
    Check(input.jumpStarted && Near(movement.velocity.y, -kJumpVelocity),
          "A08 ground jump starts on new Z press");
    input.lastKeydown = input.keydown;
    system.Run(&view);
    Check(!input.jumpStarted, "A08 holding Z must not repeatedly jump");

    // Releasing Z early should shorten the jump.
    input.lastKeydown = input.keydown;
    input.keydown.reset(Key::Z);
    system.Run(&view);
    Check(Near(movement.velocity.y,
               -kJumpVelocity * kJumpReleaseMultiplier),
          "A09 jump release multiplier");

    // Dash keeps its direction and lasts multiple updates.
    input.lastKeydown = input.keydown;
    input.keydown.set(Key::C);
    system.Run(&view);
    Check(Near(movement.velocity.x, kDashVelocity) &&
              input.lastDash == kDashInterval,
          "A09 dash speed and cooldown");
    input.lastKeydown = input.keydown;
    system.Run(&view);
    Check(Near(movement.velocity.x, kDashVelocity) &&
              input.dashFramesRemaining > 0,
          "A09 dash duration");

    // Simulate a new airborne left-wall contact and ensure gravity respects the cap.
    input.keydown.reset(Key::C);
    input.keydown.set(Key::Left);
    input.lastKeydown = input.keydown;
    body.previousOnGround = 0;
    body.previousOnLeftWall = 1;
    movement.velocity.y = 250.f;
    system.Run(&view);
    gravity.Run(&view);
    Check(movement.velocity.y <= kWallSlideVelocity + 0.001f,
          "A09 wall slide must remain capped after gravity");

    input.lastKeydown = input.keydown;
    input.keydown.set(Key::Z);
    system.Run(&view);
    Check(input.jumpStarted &&
              Near(movement.velocity.x, kWallJumpVelocity) &&
              Near(movement.velocity.y, -kJumpVelocity),
          "A09 jumping off the left wall");

    // Down arrow should increase downward velocity.
    input.lastKeydown = input.keydown;
    input.keydown.reset();
    input.keydown.set(Key::Down);
    body.previousOnLeftWall = 0;
    movement.velocity.y = 20.f;
    system.Run(&view);
    Check(movement.velocity.y > 20.f,
          "A09 Down key should accelerate falling");
}

void TestA07CollisionMathAndEvents() {
    const Rect a{{0.f, 0.f}, 10.f, 10.f};
    const Rect b{{8.f, 1.f}, 5.f, 8.f};
    const Point2D overlap = CollisionMath::OverlapDepth(a, b);
    Check(CollisionMath::Overlaps(a, b) &&
              Near(overlap.x, 2.f) && Near(overlap.y, 8.f),
          "A07 AABB overlap and penetration");
    Check(!CollisionMath::Overlaps(a, {{10.f, 0.f}, 5.f, 5.f}),
          "A07 touching edges should not count as penetration");

    // Player/floor events and contact persistence.
    PhysicsView view;
    const EntityID floor = AddPhysicsBody(
        view, "floor", {0.f, 10.f},
        {{0.f, 0.f}, 100.f, 10.f}, CollisionKind::Terrain, true);
    const EntityID player = AddPhysicsBody(
        view, "player", {2.f, 2.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
        false, {0.f, 50.f});
    SetPreviousPosition(view, player, {2.f, -5.f});
    auto events = std::make_shared<CollisionEventQueue>();
    PhysicsCollision collision(events);
    Check(collision.Run(&view) == kContinue, "A07 should continue");
    Check(view.entities.GetComponent<CollisionComponent>(player).onGround,
          "A07 player should land on floor");
    Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.y, 0.f),
          "A07 player should be placed on top of floor");
    Check(Near(view.entities.GetComponent<MovementComponent>(player).velocity.y, 0.f),
          "A07 landing should remove downward velocity");
    Check(events->pending.size() == 1 &&
              events->pending[0].phase == ContactPhase::Begin,
          "A07 first contact should emit Begin");
    events->Clear();
    SetPreviousPosition(view, player,
        view.entities.GetComponent<LocationComponent>(player).loc);
    collision.Run(&view);
    Check(events->pending.size() == 1 &&
              events->pending[0].phase == ContactPhase::Stay,
          "A07 resting contact should emit Stay");

    // Fresh collider must not inherit contact history from a removed Entity ID.
    const std::uint64_t previousIdentity =
        view.entities.GetComponent<CollisionComponent>(player).instanceId;
    view.entities.RemoveEntity(player);
    const EntityID replacement = AddPhysicsBody(
        view, "player2", {2.f, 2.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
        false, {0.f, 50.f});
    Check(replacement == player, "A07 test expects ID reuse");
    SetPreviousPosition(view, replacement, {2.f, -5.f});
    events->Clear();
    collision.Run(&view);
    Check(view.entities.GetComponent<CollisionComponent>(replacement).instanceId
              != previousIdentity,
          "A07 collider instance should change after ID reuse");
    Check(events->pending.size() == 1 &&
              events->pending[0].phase == ContactPhase::Begin,
          "A07 new collider should emit a fresh Begin event");
    (void)floor;
}

void TestA07StaticWallsAndStars() {
    // Fast movement through a thin wall (Swept AABB).
    {
        PhysicsView view;
        const EntityID wall = AddPhysicsBody(
            view, "wall", {50.f, 0.f},
            {{0.f, 0.f}, 2.f, 100.f}, CollisionKind::Terrain, true);
        const EntityID player = AddPhysicsBody(
            view, "player", {100.f, 10.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
            false, {6000.f, 0.f});
        SetPreviousPosition(view, player, {0.f, 10.f});
        PhysicsCollision collision;
        collision.Run(&view);
        Check(view.entities.GetComponent<CollisionComponent>(player).onRightWall,
              "A07 right wall flag after fast movement");
        Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.x, 40.f),
              "A07 fast movement must not tunnel through wall");
        (void)wall;
    }
    // Ground hit should bounce a star up with 0.65 restitution.
    {
        PhysicsView view;
        const EntityID floor = AddPhysicsBody(
            view, "floor", {0.f, 20.f},
            {{0.f, 0.f}, 100.f, 10.f}, CollisionKind::Terrain, true);
        const EntityID star = AddPhysicsBody(
            view, "star", {0.f, 15.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Star,
            false, {0.f, 100.f});
        SetPreviousPosition(view, star, {0.f, 5.f});
        PhysicsCollision collision;
        collision.Run(&view);
        Check(Near(view.entities.GetComponent<MovementComponent>(star).velocity.y, -65.f),
              "A07 star should bounce upward with loss of speed");
        (void)floor;
    }
    // Equal-mass star-star collision reverses head-on motion.
    {
        PhysicsView view;
        const EntityID first = AddPhysicsBody(
            view, "first", {0.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Star,
            false, {100.f, 0.f});
        const EntityID second = AddPhysicsBody(
            view, "second", {8.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Star,
            false, {-100.f, 0.f});
        PhysicsCollision collision;
        collision.Run(&view);
        Check(view.entities.GetComponent<MovementComponent>(first).velocity.x < 0.f &&
              view.entities.GetComponent<MovementComponent>(second).velocity.x > 0.f,
              "A07 star-star impulse should reverse velocities");
    }
    // Player-star sensor generates event but does not delete or stop either body.
    {
        PhysicsView view;
        const EntityID player = AddPhysicsBody(
            view, "player", {0.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player);
        const EntityID star = AddPhysicsBody(
            view, "star", {5.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Star);
        auto events = std::make_shared<CollisionEventQueue>();
        PhysicsCollision collision(events);
        collision.Run(&view);
        Check(events->pending.size() == 1 &&
              events->pending[0].phase == ContactPhase::Begin,
              "A07 Player/Star event Begin");
        Check(view.entities.IsValidEntity(star),
              "A07 may not delete star (B11 owns it)");
        Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.x, 0.f),
              "A07 sensor should not move the player");
    }
}


void TestA07EdgeCases() {
    // Ceiling: upward velocity must be cancelled.
    {
        PhysicsView view;
        AddPhysicsBody(view, "ceiling", {0.f, 0.f},
                       {{0.f, 0.f}, 100.f, 10.f},
                       CollisionKind::Terrain, true);
        const EntityID player = AddPhysicsBody(view, "player", {0.f, -20.f},
                             {{0.f, 0.f}, 10.f, 10.f},
                             CollisionKind::Player, false, {0.f, -300.f});
        SetPreviousPosition(view, player, {0.f, 20.f});
        PhysicsCollision collision;
        collision.Run(&view);
        Check(view.entities.GetComponent<CollisionComponent>(player).onCeiling,
              "A07 ceiling contact flag");
        Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.y, 10.f) &&
              Near(view.entities.GetComponent<MovementComponent>(player).velocity.y, 0.f),
              "A07 ceiling corrects position and velocity");
    }
    // Left wall: normal direction is the opposite of the right wall.
    {
        PhysicsView view;
        AddPhysicsBody(view, "leftWall", {10.f, 0.f},
                       {{0.f, 0.f}, 2.f, 100.f},
                       CollisionKind::Terrain, true);
        const EntityID player = AddPhysicsBody(view, "player", {-50.f, 10.f},
                       {{0.f, 0.f}, 10.f, 10.f},
                       CollisionKind::Player, false, {-6000.f, 0.f});
        SetPreviousPosition(view, player, {40.f, 10.f});
        PhysicsCollision collision;
        collision.Run(&view);
        Check(view.entities.GetComponent<CollisionComponent>(player).onLeftWall,
              "A07 left wall contact flag");
        Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.x, 12.f),
              "A07 left wall must prevent tunnelling");
    }
    // Fast player-star sensor contact, even without overlap at the end.
    {
        PhysicsView view;
        const EntityID player = AddPhysicsBody(view, "player", {100.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player,
            false, {1000.f, 0.f});
        const EntityID star = AddPhysicsBody(view, "star", {50.f, 0.f},
            {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Star);
        SetPreviousPosition(view, player, {0.f, 0.f});
        SetPreviousPosition(view, star, {50.f, 0.f});
        auto events = std::make_shared<CollisionEventQueue>();
        PhysicsCollision collision(events);
        collision.Run(&view);
        Check(events->pending.size() == 1 &&
              events->pending[0].phase == ContactPhase::Begin,
              "A07 high-speed Player/Star contact event");
        Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.x, 100.f),
              "A07 sensor must not stop player motion");
    }
}

void TestA10WallJumpIntegration() {
    PhysicsView view;
    AddPhysicsBody(view, "rightWall", {25.f, 0.f},
                   {{0.f, 0.f}, 10.f, 100.f},
                   CollisionKind::Terrain, true);
    const EntityID player = AddPhysicsBody(view, "player", {15.f, 40.f},
                    {{0.f, 0.f}, 10.f, 10.f},
                    CollisionKind::Player, false, {0.f, 100.f});
    view.entities.AddComponent(player, InputComponent(player));
    PhysicsCollision collision;
    PhysicsLate late;
    PhysicsInput inputSystem;

    collision.Run(&view);
    Check(view.entities.GetComponent<CollisionComponent>(player).onRightWall,
          "A10 A07 must detect right wall before A09 can wall jump");
    late.Run(&view);
    Check(view.entities.GetComponent<CollisionComponent>(player)
              .previousOnRightWall,
          "A10 A04 must preserve previous wall contact");
    auto& input = view.entities.GetComponent<InputComponent>(player);
    input.keydown.set(Key::Z);
    inputSystem.Run(&view);
    const auto& velocity =
        view.entities.GetComponent<MovementComponent>(player).velocity;
    Check(input.jumpStarted && Near(velocity.x, -kWallJumpVelocity) &&
              Near(velocity.y, -kJumpVelocity),
          "A10 A07 to A09 wall-jump integration");
}

void TestA10Pipeline() {
    PhysicsView view;
    const EntityID floor = AddPhysicsBody(
        view, "floor", {0.f, 20.f},
        {{0.f, 0.f}, 100.f, 10.f}, CollisionKind::Terrain, true);
    const EntityID player = AddPhysicsBody(
        view, "player", {10.f, 10.f},
        {{0.f, 0.f}, 10.f, 10.f}, CollisionKind::Player);
    view.entities.AddComponent(player, InputComponent(player));
    view.entities.AddComponent(
        player, GravityComponent(Point2D(0.f, 900.f)));
    PhysicsLate late;
    PhysicsInput input;
    PhysicsGravity gravity;
    PhysicsMovement movement;
    PhysicsCollision collision;

    // The real system order: Late -> Input -> Gravity -> Movement -> Collision.
    auto frame = [&]() {
        late.Run(&view);
        input.Run(&view);
        gravity.Run(&view);
        movement.Run(&view);
        collision.Run(&view);
        auto& keys = view.entities.GetComponent<InputComponent>(player);
        keys.lastKeydown = keys.keydown;
    };

    frame();
    Check(view.entities.GetComponent<CollisionComponent>(player).onGround,
          "A10 first frame should keep player standing on ground");
    Check(Near(view.entities.GetComponent<LocationComponent>(player).loc.y, 10.f),
          "A10 landing must correct the position");

    auto& keys = view.entities.GetComponent<InputComponent>(player);
    keys.keydown.set(Key::Z);
    frame();
    Check(keys.jumpStarted &&
              view.entities.GetComponent<MovementComponent>(player).velocity.y < 0.f,
          "A10 Z must jump after real collision detection");
    Check(!view.entities.GetComponent<CollisionComponent>(player).onGround,
          "A10 player must leave the floor when jumping");
    const float risingVelocity =
        view.entities.GetComponent<MovementComponent>(player).velocity.y;
    keys.keydown.reset(Key::Z);
    frame();
    Check(view.entities.GetComponent<MovementComponent>(player).velocity.y > risingVelocity,
          "A10 early Z release must shorten upward motion");
    (void)floor;
}

int main() {
    try {
        TestAllComponentTypes();
        TestMultipleEntitiesAndHoles();
        TestInvalidEntities();
        TestComponentLifetime();
        TestA04CollisionLate();
        TestA05Gravity();
        TestA06Movement();
        TestA08AndA09Input();
        TestA07CollisionMathAndEvents();
        TestA07StaticWallsAndStars();
        TestA07EdgeCases();
        TestA10WallJumpIntegration();
        TestA10Pipeline();
        std::cout << "A02-A10 EngineTest passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "EngineTest failed: " << error.what() << '\n';
        return 1;
    }
}
