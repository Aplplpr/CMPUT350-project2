#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>
#include "CollisionComponent.h"
#include "CollisionEvents.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>
#include "CollisionComponent.h"
#include "CollisionEvents.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"
namespace ECSEngine {

namespace CollisionMath {
constexpr float kEpsilon = 0.01f;
inline Rect WorldBounds(const LocationComponent& location, const CollisionComponent& collision) {
    Rect bounds = collision.collisionBox;
    bounds.topLeft.x += location.loc.x;
    bounds.topLeft.y += location.loc.y;
    return bounds;
}

inline bool Overlaps(const Rect& a, const Rect& b) {
    if (a.width <= 0.f || a.height <= 0.f || b.width <= 0.f || b.height <= 0.f) {
        return false;
    }
    return a.topLeft.x < b.topLeft.x + b.width &&
           a.topLeft.x + a.width > b.topLeft.x &&
           a.topLeft.y < b.topLeft.y + b.height &&
           a.topLeft.y + a.height > b.topLeft.y;
}

inline Point2D OverlapDepth(const Rect& a, const Rect& b) {
    if (!Overlaps(a, b)) {
        return Point2D(0.f, 0.f);
    }

    float x = std::min(a.topLeft.x + a.width, b.topLeft.x + b.width) - std::max(a.topLeft.x, b.topLeft.x);
    float y = std::min(a.topLeft.y + a.height, b.topLeft.y + b.height) -std::max(a.topLeft.y, b.topLeft.y);
    return Point2D(x, y);
}
struct SweepHit {
    bool hit = false;
    float time = 1.f;
    Point2D normal{};
};

inline SweepHit SweptAABB(const Rect& startA, const Rect& endA,const Rect& startB, const Rect& endB) {
    if (startA.width <= 0.f || startA.height <= 0.f || startB.width <= 0.f || startB.height <= 0.f) {
        return {};
    }
    // Initial penetration is handled separately.
    if (Overlaps(startA, startB)) {
        return {};
    }
    // Movement of A relative to B.
    const float dx = (endA.topLeft.x - startA.topLeft.x) -(endB.topLeft.x - startB.topLeft.x);

    const float dy = (endA.topLeft.y - startA.topLeft.y) - (endB.topLeft.y - startB.topLeft.y);

    const float infinity = std::numeric_limits<float>::infinity();
    float entryX = -infinity;
    float exitX = infinity;
    float entryY = -infinity;
    float exitY = infinity;

    // Time interval of overlap along the X axis.
    if (dx > 0.f) {
        entryX =(startB.topLeft.x -(startA.topLeft.x + startA.width)) / dx;
        exitX =(startB.topLeft.x + startB.width - startA.topLeft.x) / dx;
    } else if (dx < 0.f) {
        entryX =(startB.topLeft.x + startB.width - startA.topLeft.x) / dx;
        exitX =(startB.topLeft.x - (startA.topLeft.x + startA.width)) / dx;

    } else if (startA.topLeft.x + startA.width <= startB.topLeft.x || startA.topLeft.x >= startB.topLeft.x + startB.width) {
        return {};
    }
    // Time interval of overlap along the Y axis.
    if (dy > 0.f) {
        entryY =(startB.topLeft.y -(startA.topLeft.y + startA.height)) / dy;
        exitY =(startB.topLeft.y + startB.height -startA.topLeft.y) / dy;
    } 
    else if (dy < 0.f) {
        entryY =(startB.topLeft.y + startB.height -startA.topLeft.y) / dy;
        exitY =(startB.topLeft.y - (startA.topLeft.y + startA.height)) / dy;

    } 
    else if (startA.topLeft.y + startA.height <= startB.topLeft.y ||startA.topLeft.y >= startB.topLeft.y + startB.height) {
        return {};
    }
    const float entryTime = std::max(entryX, entryY);
    const float exitTime = std::min(exitX, exitY);

    if (entryTime > exitTime || entryTime < 0.f || entryTime > 1.f || exitTime < 0.f) {
        return {};
    }
    SweepHit result;
    result.hit = true;
    result.time = entryTime;
    if (entryX > entryY) {
        result.normal.x = dx > 0.f ? -1.f : 1.f;
    } else {
        result.normal.y = dy > 0.f ? -1.f : 1.f;
    }

    return result;
}

inline void ApplyNormal(CollisionComponent& body,  MovementComponent& movement,const Point2D& normal) {
    if (normal.x < 0.f) {
        body.onRightWall = 1;
        movement.velocity.x = std::min(movement.velocity.x, 0.f);
    } 
    else if (normal.x > 0.f) {
        body.onLeftWall = 1;
        movement.velocity.x = std::max(movement.velocity.x, 0.f);
    }
    if (normal.y < 0.f) {body.onGround = 1;movement.velocity.y = std::min(movement.velocity.y, 0.f);
    } 
    else if (normal.y > 0.f) {
        body.onCeiling = 1;
        movement.velocity.y = std::max(movement.velocity.y, 0.f);
    }
}

inline void ResolveStatic(LocationComponent& location, MovementComponent& movement, CollisionComponent& body, const Rect& obstacle) {
    const Rect now = WorldBounds(location, body);
    if (now.width <= 0.f || now.height <= 0.f ||obstacle.width <= 0.f || obstacle.height <= 0.f) {
        return;
    }
    const Rect before = body.hasPreviousCollisionBox ? body.previousCollisionBox : now;
    const SweepHit sweep = SweptAABB(before, now, obstacle, obstacle);
    if (sweep.hit) {
        if (sweep.normal.x != 0.f) {
            const float hitX = before.topLeft.x + (now.topLeft.x - before.topLeft.x) * sweep.time;
            location.loc.x += hitX - now.topLeft.x;
        } 
        else {
            const float hitY = before.topLeft.y + (now.topLeft.y - before.topLeft.y) * sweep.time;
            location.loc.y += hitY - now.topLeft.y;
        }
        ApplyNormal(body, movement, sweep.normal);
        return;
    }
    if (!Overlaps(now, obstacle)) {
        return;
    }
    const Point2D depth = OverlapDepth(now, obstacle);
    const float centerAx = now.topLeft.x + now.width * 0.5f;
    const float centerAy = now.topLeft.y + now.height * 0.5f;
    const float centerBx = obstacle.topLeft.x + obstacle.width * 0.5f;
    const float centerBy = obstacle.topLeft.y + obstacle.height * 0.5f;
    if (depth.x < depth.y) {
        const float normalX = centerAx < centerBx ? -1.f : 1.f;
        location.loc.x += normalX * depth.x;
        ApplyNormal(body, movement, Point2D(normalX, 0.f));
    } 
    else {
        const float normalY = centerAy < centerBy ? -1.f : 1.f;
        location.loc.y += normalY * depth.y;
        ApplyNormal(body, movement, Point2D(0.f, normalY));
    }
}

inline void MarkRestingContact(CollisionComponent& body,const MovementComponent& movement, const Rect& now, const Rect& obstacle) {
    constexpr float kEpsilon = 0.01f;
    if (now.width <= 0.f || now.height <= 0.f || obstacle.width <= 0.f || obstacle.height <= 0.f) {
        return;
    }
    const float left = now.topLeft.x;
    const float right = left + now.width;
    const float top = now.topLeft.y;
    const float bottom = top + now.height;
    const float otherLeft = obstacle.topLeft.x;
    const float otherRight = otherLeft + obstacle.width;
    const float otherTop = obstacle.topLeft.y;
    const float otherBottom = otherTop + obstacle.height;
    const bool overlapX = left < otherRight && right > otherLeft;
    const bool overlapY =top < otherBottom && bottom > otherTop;
    if (overlapX &&std::fabs(bottom - otherTop) <= kEpsilon && movement.velocity.y >= 0.f) {
        body.onGround = 1;
    }
    if (overlapX &&std::fabs(top - otherBottom) <= kEpsilon && movement.velocity.y <= 0.f) {
        body.onCeiling = 1;
    }
    if (overlapY &&std::fabs(right - otherLeft) <= kEpsilon &&movement.velocity.x >= 0.f) {
        body.onRightWall = 1;
    }

    if (overlapY &&std::fabs(left - otherRight) <= kEpsilon &&movement.velocity.x <= 0.f) {
        body.onLeftWall = 1;
    }
}
struct Hit {
    bool valid = false;
    bool sweep = false;
    bool resting = false;
    float time = 1.f;
    float penetration = 0.f;
    Point2D normal{};
};

inline Hit FindHit(const Rect& before,
                   const Rect& after,
                   const Rect& obstacle) {
    Hit hit;

    if (before.width <= 0.f || before.height <= 0.f ||
        obstacle.width <= 0.f || obstacle.height <= 0.f) {
        return hit;
    }

    constexpr float epsilon = 0.01f;

    const float left = after.topLeft.x;
    const float right = left + after.width;
    const float top = after.topLeft.y;
    const float bottom = top + after.height;

    const float bLeft = obstacle.topLeft.x;
    const float bRight = bLeft + obstacle.width;
    const float bTop = obstacle.topLeft.y;
    const float bBottom = bTop + obstacle.height;

    // Resolve existing penetration.
    if (Overlaps(after, obstacle)) {
        const Point2D depth = OverlapDepth(after, obstacle);
        hit.valid = true;
        const float oldLeft = before.topLeft.x;
        const float oldRight = oldLeft + before.width;
        const float oldTop = before.topLeft.y;
        const float oldBottom = oldTop + before.height;
        if (oldBottom <= bTop + epsilon) {
            hit.normal = {0.f, -1.f};
            hit.penetration = bottom - bTop;
        } 
        else if (oldTop >= bBottom - epsilon) {
            hit.normal = {0.f, 1.f};
            hit.penetration = bBottom - top;
        } 
        else if (oldRight <= bLeft + epsilon) {
            hit.normal = {-1.f, 0.f};
            hit.penetration = right - bLeft;
        } 
        else if (oldLeft >= bRight - epsilon) {
            hit.normal = {1.f, 0.f};
            hit.penetration = bRight - left;
        } 
        else if (depth.y <= depth.x) {
            const float centerA = top + after.height * 0.5f;
            const float centerB = bTop + obstacle.height * 0.5f;
            hit.normal = {0.f, centerA < centerB ? -1.f : 1.f};
            hit.penetration = depth.y;
        } 
        else {
            const float centerA = left + after.width * 0.5f;
            const float centerB = bLeft + obstacle.width * 0.5f;
            hit.normal = {centerA < centerB ? -1.f : 1.f, 0.f};
            hit.penetration = depth.x;
        }
        return hit;
    }
    // Preserve contact when surfaces are touching.
    const bool overlapX = left < bRight && right > bLeft;
    const bool overlapY = top < bBottom && bottom > bTop;
    const float dx = after.topLeft.x - before.topLeft.x;
    const float dy = after.topLeft.y - before.topLeft.y;
    if (overlapX && std::fabs(bottom - bTop) <= epsilon && dy >= -epsilon) {
        hit.valid = true;
        hit.resting = true;
        hit.normal = {0.f, -1.f};
        return hit;
    }

    if (overlapX && std::fabs(top - bBottom) <= epsilon && dy <= epsilon) {
        hit.valid = true;
        hit.resting = true;
        hit.normal = {0.f, 1.f};
        return hit;
    }
    if (overlapY &&std::fabs(right - bLeft) <= epsilon && dx >= -epsilon) {
        hit.valid = true;
        hit.resting = true;
        hit.normal = {-1.f, 0.f};
        return hit;
    }
    if (overlapY &&std::fabs(left - bRight) <= epsilon &&dx <= epsilon) {
        hit.valid = true;
        hit.resting = true;
        hit.normal = {1.f, 0.f};
        return hit;
    }
    // Check movement through an obstacle.
    const SweepHit sweep = SweptAABB(before, after, obstacle, obstacle);
    if (sweep.hit) {
        hit.valid = true;
        hit.sweep = true;
        hit.time = sweep.time;
        hit.normal = sweep.normal;
    }
    return hit;
}

inline ContactSide SideFromNormal(const Point2D& normal) {
    if (normal.y < 0.f) return ContactSide::Ground;
    if (normal.y > 0.f) return ContactSide::Ceiling;
    if (normal.x < 0.f) return ContactSide::RightWall;
    if (normal.x > 0.f) return ContactSide::LeftWall;
    return ContactSide::None;
}
inline void SetSide(CollisionComponent& collision, const Point2D& normal) {
    switch (SideFromNormal(normal)) {
        case ContactSide::Ground:
            collision.onGround = 1;
            break;
        case ContactSide::Ceiling:
            collision.onCeiling = 1;
            break;
        case ContactSide::LeftWall:
            collision.onLeftWall = 1;
            break;
        case ContactSide::RightWall:
            collision.onRightWall = 1;
            break;
        default:
            break;
    }
}


}  // namespace CollisionMath

struct CollisionPairKey {
    EntityID a;
    EntityID b;
    std::uint64_t instanceA;
    std::uint64_t instanceB;

    bool operator==(const CollisionPairKey& other) const {
        return a == other.a &&b == other.b && instanceA == other.instanceA && instanceB == other.instanceB;
    }
};
struct CollisionPairHash {
    std::size_t operator()(const CollisionPairKey& key) const {
        std::size_t result = std::hash<EntityID>{}(key.a);
        auto combine = [&result](std::size_t value) {
            result ^= value + 0x9e3779b9 + (result << 6) + (result >> 2);
        };
        combine(std::hash<EntityID>{}(key.b));
        combine(std::hash<std::uint64_t>{}(key.instanceA));
        combine(std::hash<std::uint64_t>{}(key.instanceB));
        return result;
    }
};

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    explicit CollisionSystem(std::shared_ptr<CollisionEventQueue> events = nullptr): mEvents(std::move(events)) {}
    const std::string& GetName() const override {
        static const std::string name("CollisionSystem");
        return name;
    }
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& manager = engine->GetEntityManager();
        auto& colliders = manager.template GetComponentStorage<CollisionComponent>();
        std::unordered_set<CollisionPairKey, CollisionPairHash> current;
        std::vector<EntityID> dynamics;
        std::vector<EntityID> statics;
        for (EntityID id : colliders.mOwners) {
            if (!manager.IsValidEntity(id) || !manager.template HasComponent<LocationComponent>(id)) {
                continue;
            }
            const auto& collision =manager.template GetComponent<CollisionComponent>(id);
            if (collision.staticEntity) {
                statics.push_back(id);
            } else {
                dynamics.push_back(id);
            }
        }

        auto isTopCovered = [&](EntityID fixedID, const Rect& body, const Rect& obstacle) {
            constexpr float epsilon = 0.01f;
            const float contactLeft = std::max(body.topLeft.x, obstacle.topLeft.x);
            const float contactRight = std::min( body.topLeft.x + body.width,obstacle.topLeft.x + obstacle.width);
            if (contactRight <= contactLeft) {
                return false;
            }
            for (EntityID otherID : statics) {
                if (otherID == fixedID) {
                    continue;
                }
                const auto& otherCollision = manager.template GetComponent<CollisionComponent>(otherID);
                const auto& otherLocation = manager.template GetComponent<LocationComponent>(otherID);
                const Rect other = CollisionMath::WorldBounds(otherLocation, otherCollision);
                const float otherBottom = other.topLeft.y + other.height;
                if (std::fabs(otherBottom - obstacle.topLeft.y) > epsilon) {
                    continue;
                }
                const float otherRight = other.topLeft.x + other.width;
                if (other.topLeft.x <= contactLeft + epsilon &&otherRight >= contactRight - epsilon) {
                    return true;
                }
            }
            return false;
        };

        auto processPair = [&](EntityID idA, EntityID idB) {
            auto& a = manager.template GetComponent<CollisionComponent>(idA);
            auto& b =manager.template GetComponent<CollisionComponent>(idB);
            if (a.staticEntity && b.staticEntity) {
                return;
            }
            auto& locA =manager.template GetComponent<LocationComponent>(idA);
            auto& locB = manager.template GetComponent<LocationComponent>(idB);
            const Rect nowA = CollisionMath::WorldBounds(locA, a);
            const Rect nowB = CollisionMath::WorldBounds(locB, b);
            if (nowA.width <= 0.f || nowA.height <= 0.f ||nowB.width <= 0.f || nowB.height <= 0.f) {
                return;
            }
            const bool collectible =(a.kind == CollisionKind::Player && b.kind == CollisionKind::Star) ||(a.kind == CollisionKind::Star && b.kind == CollisionKind::Player);
            bool contacting = false;
            Point2D normalForA{};
            float impact = 0.f;
            if (collectible) {
                // Player-star is a sensor: no physical push or bounce.
                const Rect beforeA = a.hasPreviousCollisionBox ? a.previousCollisionBox : nowA;
                const Rect beforeB = b.hasPreviousCollisionBox ? b.previousCollisionBox : nowB;
                Rect relativeEnd = beforeA;
                relativeEnd.topLeft.x += (nowA.topLeft.x - beforeA.topLeft.x) -(nowB.topLeft.x - beforeB.topLeft.x);
                relativeEnd.topLeft.y += (nowA.topLeft.y - beforeA.topLeft.y) -(nowB.topLeft.y - beforeB.topLeft.y);
                const auto sweep =CollisionMath::SweptAABB(beforeA, relativeEnd, beforeB, beforeB);
                contacting = CollisionMath::Overlaps(nowA, nowB) || sweep.hit;
                if (contacting) {
                    if (sweep.hit) {
                        normalForA = sweep.normal;
                    } 
                    else {
                        const Point2D depth = CollisionMath::OverlapDepth(nowA, nowB);
                        const float centerAx =  nowA.topLeft.x + nowA.width * 0.5f;
                        const float centerBx = nowB.topLeft.x + nowB.width * 0.5f;
                        const float centerAy = nowA.topLeft.y + nowA.height * 0.5f;
                        const float centerBy = nowB.topLeft.y + nowB.height * 0.5f;
                        if (depth.x < depth.y) {
                            normalForA = { centerAx < centerBx ? -1.f : 1.f, 0.f};
                        } 
                        else {
                            normalForA = { 0.f, centerAy < centerBy ? -1.f : 1.f};
                        }
                    }
                    Point2D va{};
                    Point2D vb{};
                    if (manager.template HasComponent<MovementComponent>(idA)) {
                        va = manager.template GetComponent<MovementComponent>(idA).velocity;
                    }
                    if (manager.template HasComponent<MovementComponent>(idB)) {
                        vb = manager.template GetComponent<MovementComponent>(idB).velocity;
                    }
                    impact = std::hypot(va.x - vb.x, va.y - vb.y);
                }
            } 
            else if (a.staticEntity != b.staticEntity) {
                // One dynamic body and one static obstacle.
                const bool movingA = !a.staticEntity;
                auto& moving = movingA ? a : b;
                const Rect movingNow = movingA ? nowA : nowB;
                const Rect obstacle = movingA ? nowB : nowA;
                const Rect before =moving.hasPreviousCollisionBox ? moving.previousCollisionBox : movingNow;
                auto hit =CollisionMath::FindHit(before, movingNow, obstacle);
                const EntityID fixedID = movingA ? idB : idA;
                if (hit.valid && moving.kind == CollisionKind::Player && hit.normal.y < 0.f && isTopCovered(fixedID, movingNow, obstacle)) {
                    const float bodyCenter = movingNow.topLeft.x + movingNow.width * 0.5f;
                    const float obstacleCenter = obstacle.topLeft.x + obstacle.width * 0.5f;
                    const bool fromLeft = bodyCenter < obstacleCenter;
                    hit.normal = fromLeft? Point2D(-1.f, 0.f): Point2D(1.f, 0.f);
                    const float bodyLeft = movingNow.topLeft.x;
                    const float bodyRight = bodyLeft + movingNow.width;
                    const float obstacleLeft = obstacle.topLeft.x;
                    const float obstacleRight =obstacleLeft + obstacle.width;
                    hit.penetration = fromLeft? bodyRight - obstacleLeft : obstacleRight - bodyLeft;
                    hit.sweep = false;
                    hit.resting = false;
                    if (hit.penetration <= 0.f) {
                        hit.valid = false;
                    }
                }
                contacting = hit.valid;
                if (contacting) {
                    const EntityID movingID = movingA ? idA : idB;
                    normalForA = movingA? hit.normal: Point2D(-hit.normal.x, -hit.normal.y);
                    auto& movingLocation = manager.template GetComponent<LocationComponent>( movingID );
                    if (hit.sweep) {
                        const float remaining = 1.f - hit.time;
                        if (hit.normal.x != 0.f) {
                            movingLocation.loc.x +=(before.topLeft.x - movingNow.topLeft.x) * remaining;
                        } 
                        else if (hit.normal.y != 0.f) {
                            movingLocation.loc.y += (before.topLeft.y - movingNow.topLeft.y) *remaining;
                        }
                    }
                    else if (!hit.resting) {
                        movingLocation.loc.x += hit.normal.x * hit.penetration;
                        movingLocation.loc.y += hit.normal.y * hit.penetration;
                    }
                    if (manager.template HasComponent<MovementComponent>(movingID)) {
                        auto& velocity = manager.template GetComponent<MovementComponent>( movingID).velocity;
                        const float vn = velocity.x * hit.normal.x + velocity.y * hit.normal.y;
                        impact = std::max(0.f, -vn);
                        if (vn < 0.f) {
                            const float restitution = moving.kind == CollisionKind::Star ? 0.65f : 0.f;
                            velocity.x -=(1.f + restitution) *vn * hit.normal.x;
                            velocity.y -= (1.f + restitution) * vn * hit.normal.y;
                        }
                    }
                    CollisionMath::SetSide(moving, hit.normal);
                    auto& fixed = movingA ? b : a;
                    CollisionMath::SetSide(fixed, Point2D(-hit.normal.x, -hit.normal.y));
                }
            } 
            else {
                // Dynamic-dynamic collision.
                const Rect beforeA =  a.hasPreviousCollisionBox ? a.previousCollisionBox: nowA;
                const Rect beforeB =b.hasPreviousCollisionBox ? b.previousCollisionBox: nowB;
                Rect relativeEnd = beforeA;
                relativeEnd.topLeft.x +=(nowA.topLeft.x - beforeA.topLeft.x) - (nowB.topLeft.x - beforeB.topLeft.x);
                relativeEnd.topLeft.y +=(nowA.topLeft.y - beforeA.topLeft.y) -(nowB.topLeft.y - beforeB.topLeft.y);
                const auto sweep =CollisionMath::SweptAABB( beforeA, relativeEnd, beforeB, beforeB );
                const bool overlapping =CollisionMath::Overlaps(nowA, nowB);
                contacting = overlapping || sweep.hit;
                if (contacting) {
                    if (overlapping) {
                        const Point2D depth = CollisionMath::OverlapDepth(nowA, nowB);
                        if (depth.x < depth.y) {
                            const float centerA = nowA.topLeft.x + nowA.width * 0.5f;
                            const float centerB = nowB.topLeft.x + nowB.width * 0.5f;
                            normalForA = {centerA < centerB ? -1.f : 1.f, 0.f};
                        } 
                        else {
                            const float centerA = nowA.topLeft.y + nowA.height * 0.5f;
                            const float centerB =nowB.topLeft.y + nowB.height * 0.5f;
                            normalForA = { 0.f, centerA < centerB ? -1.f : 1.f};
                        }
                        const float penetration =normalForA.x != 0.f ? depth.x : depth.y;
                        locA.loc.x += normalForA.x * penetration * 0.5f;
                        locA.loc.y += normalForA.y * penetration * 0.5f;
                        locB.loc.x -= normalForA.x * penetration * 0.5f;
                        locB.loc.y -= normalForA.y * penetration * 0.5f;
                    } 
                    else {
                        normalForA = sweep.normal;
                        const float remaining = 1.f - sweep.time;
                        locA.loc.x -= (nowA.topLeft.x - beforeA.topLeft.x) * remaining;
                        locA.loc.y -=(nowA.topLeft.y - beforeA.topLeft.y) * remaining;
                        locB.loc.x -= (nowB.topLeft.x - beforeB.topLeft.x) * remaining;
                        locB.loc.y -= (nowB.topLeft.y - beforeB.topLeft.y) * remaining;
                    }
                    if (manager.template HasComponent<MovementComponent>(idA) &&
                        manager.template HasComponent<MovementComponent>(idB)) {
                        auto& va = manager.template  GetComponent<MovementComponent>(idA).velocity;
                        auto& vb = manager.template GetComponent<MovementComponent>(idB).velocity;
                        const float relative = (va.x - vb.x) * normalForA.x + (va.y - vb.y) * normalForA.y;
                        impact = std::max(0.f, -relative);
                        if (relative < 0.f) {
                            const float impulse = -(1.f + 0.65f) * relative * 0.5f;
                            va.x += impulse * normalForA.x;
                            va.y += impulse * normalForA.y;
                            vb.x -= impulse * normalForA.x;
                            vb.y -= impulse * normalForA.y;
                        }
                    }
                    CollisionMath::SetSide(a, normalForA);
                    CollisionMath::SetSide(b, Point2D(-normalForA.x, -normalForA.y));
                }
            }
            if (!contacting) {
                return;
            }
            // Canonical order: smaller EntityID first.
            const bool swapped = idB < idA;
            const CollisionPairKey key = swapped? CollisionPairKey{idB, idA, b.instanceId, a.instanceId}: CollisionPairKey{idA, idB, a.instanceId, b.instanceId};
            // Only one event per pair during a single update.
            if (!current.insert(key).second) {
                return;
            }
            if (mEvents) {
                const ContactPhase phase = mPreviousContacts.count(key)? ContactPhase::Stay: ContactPhase::Begin;
                const Point2D normal = swapped? Point2D(-normalForA.x, -normalForA.y): normalForA;
                mEvents->pending.push_back(CollisionEvent{
                    key.a,
                    key.b,
                    key.instanceA,
                    key.instanceB,
                    swapped ? b.kind : a.kind,
                    swapped ? a.kind : b.kind,
                    CollisionMath::SideFromNormal(normal),
                    phase,
                    impact,
                    static_cast<bool>( a.doesDamage || b.doesDamage)});
            }
        };

        // Dynamic-static pairs.
        for (EntityID dynamic : dynamics) {
            for (EntityID fixed : statics) {
                processPair(dynamic, fixed);
            }
        }
        // Dynamic-dynamic pairs, each processed once.
        for (std::size_t i = 0; i < dynamics.size(); ++i) {
            for (std::size_t j = i + 1; j < dynamics.size(); ++j) {
                processPair(dynamics[i], dynamics[j]);
            }
        }
        mPreviousContacts.swap(current);
        // Recheck the player's contact flags using final positions.
        for (EntityID dynamicID : dynamics) {
            auto& body = manager.template GetComponent<CollisionComponent>(dynamicID);
            if (body.kind != CollisionKind::Player || !manager.template HasComponent<MovementComponent>(dynamicID)) {
                continue;
            }
            body.onGround = 0;
            body.onCeiling = 0;
            body.onLeftWall = 0;
            body.onRightWall = 0;
            const auto& location = manager.template GetComponent<LocationComponent>(dynamicID);
            const auto& movement = manager.template GetComponent<MovementComponent>(dynamicID);
            const Rect finalBounds =CollisionMath::WorldBounds(location, body);
            for (EntityID fixedID : statics) {
                const auto& fixedBody = manager.template GetComponent<CollisionComponent>(fixedID);
                const auto& fixedLocation = manager.template GetComponent<LocationComponent>(fixedID);
                const Rect obstacle =CollisionMath::WorldBounds(fixedLocation, fixedBody);
                CollisionMath::MarkRestingContact(body, movement, finalBounds, obstacle);
            }
        }
        return kContinue;
    }

private:
    std::shared_ptr<CollisionEventQueue> mEvents;
    std::unordered_set<CollisionPairKey, CollisionPairHash>mPreviousContacts;
};  // namespace ECSEngine
}