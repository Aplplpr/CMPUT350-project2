#pragma once
#include <cstdint>
#include "MathUtil.h"
namespace ECSEngine {

struct CollisionComponent {
    std::uint8_t staticEntity : 1 = 0;
    std::uint8_t doesDamage : 1 = 0;
    std::uint8_t onGround : 1 = 0;
    std::uint8_t onCeiling : 1 = 0;
    std::uint8_t onLeftWall : 1 = 0;
    std::uint8_t onRightWall : 1 = 0;
    // Previous frame collision state
    std::uint8_t previousOnGround : 1 = 0;
    std::uint8_t previousOnCeiling : 1 = 0;
    std::uint8_t previousOnLeftWall : 1 = 0;
    std::uint8_t previousOnRightWall : 1 = 0;
    // Local-space collision bounds relative to LocationComponent
    Rect collisionBox{};
    // World-space collision bounds before the current frame movement
    Rect previousCollisionBox{};
    // Whether previousCollisionBox contains a valid snapshot
    bool hasPreviousCollisionBox = false;
};

}  // namespace ECSEngine
