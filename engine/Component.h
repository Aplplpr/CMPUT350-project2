#pragma once
#include <vector>
#include "Entity.h"
namespace ECSEngine {

template <typename T>
class ComponentStorage {
public:
    std::vector<T> mStorage;
    std::vector<EntityID> mOwners;
};

}  // namespace ECSEngine
