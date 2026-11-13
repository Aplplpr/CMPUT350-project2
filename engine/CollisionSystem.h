
#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

namespace CollisionMath {

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

}  // namespace CollisionMath

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static const std::string name("CollisionSystem");
        return name;
    }
    tSystemReturn Run(EngineView<Components...>* engine) override {
        (void)engine;
        return kContinue;
    }
};
}  // namespace ECSEngine
