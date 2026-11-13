#pragma once
#include <cstdint>
#include <atomic>
#include "MathUtil.h"
namespace ECSEngine {
enum class CollisionKind : std::uint8_t { Other, Player, Star, Terrain };
struct CollisionComponent {
    inline static std::atomic<std::uint64_t> nextInstanceId{1};
    std::uint64_t instanceId = nextInstanceId.fetch_add(1, std::memory_order_relaxed);
    CollisionKind kind = CollisionKind::Other;
    std::uint8_t staticEntity : 1 = 0;
    std::uint8_t doesDamage : 1 = 0;
    std::uint8_t onGround : 1 = 0;
    std::uint8_t onCeiling : 1 = 0;
    std::uint8_t onLeftWall : 1 = 0;
    std::uint8_t onRightWall : 1 = 0;
    std::uint8_t previousOnGround : 1 = 0;
    std::uint8_t previousOnCeiling : 1 = 0;
    std::uint8_t previousOnLeftWall : 1 = 0;
    std::uint8_t previousOnRightWall : 1 = 0;
    Rect collisionBox{};              // Local to LocationComponent
    Rect previousCollisionBox{};      // World space, from start of physics step
    bool hasPreviousCollisionBox = false;
};

}  // namespace ECSEngine
