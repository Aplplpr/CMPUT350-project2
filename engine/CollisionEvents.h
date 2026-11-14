#pragma once

#include <cstdint>
#include <vector>
#include "CollisionComponent.h"
#include "Entity.h"

namespace ECSEngine {

enum class ContactSide : std::uint8_t { None, Ground, Ceiling, LeftWall, RightWall };
enum class ContactPhase : std::uint8_t { Begin, Stay };

struct CollisionEvent {
    EntityID entityA{};             // Lower numeric entity ID
    EntityID entityB{};             // Higher numeric entity ID
    std::uint64_t instanceA{};     // Distinguishes a reused EntityID
    std::uint64_t instanceB{};
    CollisionKind kindA = CollisionKind::Other;
    CollisionKind kindB = CollisionKind::Other;
    ContactSide sideForA = ContactSide::None;
    ContactPhase phase = ContactPhase::Begin;
    float impactSpeed = 0.f;
    bool damaging = false;
};

// Created by the scene and shared with CollisionSystem (producer) and
// B11 event processor (consumer). ONLY B11 clears pending after consumption.
struct CollisionEventQueue {
    std::vector<CollisionEvent> pending;
    void Clear() { pending.clear(); }
};

}  // namespace ECSEngine
