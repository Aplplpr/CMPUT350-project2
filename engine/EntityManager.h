#pragma once

#include <array>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent =std::numeric_limits<size_t>::max();
    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        if (!mFreeEntities.empty()) {
            EntityID id = mFreeEntities.back();
            mFreeEntities.pop_back();
            mEntities[id] = Entity(id, name);
            mValidEntities[id] = true;
            mComponentIndices[id].fill(kNullComponent);
            return id;
        }
        EntityID id = mEntities.size();
        mEntities.push_back(Entity(id, name));
        mValidEntities.push_back(true);
        std::array<size_t, sizeof...(Components)> indices;
        indices.fill(kNullComponent);
        mComponentIndices.push_back(indices);
        return id;
    }

    void RemoveEntity(EntityID entity) {
        if (!IsValidEntity(entity)) return;
        (RemoveComponent<Components>(entity), ...);
        mValidEntities[entity] = false;
        mFreeEntities.push_back(entity);
    }
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        if (!IsValidEntity(entity)) {
            throw std::out_of_range("Invalid entity ID");
        }
        constexpr size_t typeIndex =IndexOf<T, Components...>();
        auto& storage =std::get<ComponentStorage<T>>(mComponentStorages);
        size_t& slot =mComponentIndices[entity][typeIndex];
        if (slot != kNullComponent) {
            storage.mStorage[slot] = std::move(component);
            return;
        }
        slot = storage.mStorage.size();
        storage.mStorage.push_back(std::move(component));
        storage.mOwners.push_back(entity);
    }
    template <typename T>
    void RemoveComponent(EntityID entity) {
        if (!IsValidEntity(entity)) return;
        constexpr size_t typeIndex =IndexOf<T, Components...>();
        size_t& slot =mComponentIndices[entity][typeIndex];
        if (slot == kNullComponent) return;
        auto& storage =std::get<ComponentStorage<T>>(mComponentStorages);
        const size_t last = storage.mStorage.size() - 1;
        if (slot != last) {
            storage.mStorage[slot] =std::move(storage.mStorage[last]);
            const EntityID movedOwner =storage.mOwners[last];
            storage.mOwners[slot] = movedOwner;
            mComponentIndices[movedOwner][typeIndex] = slot;
        }
        storage.mStorage.pop_back();
        storage.mOwners.pop_back();
        slot = kNullComponent;
    }
    template <typename T>
    bool HasComponent(EntityID entity) const {
        if (!IsValidEntity(entity)) return false;
        constexpr size_t typeIndex = IndexOf<T, Components...>();
        return mComponentIndices[entity][typeIndex]!= kNullComponent;
    }
    template <typename T>
    T& GetComponent(EntityID entity) {
        if (!HasComponent<T>(entity)) {
            throw std::out_of_range("Missing component or invalid entity");
        }
        constexpr size_t typeIndex = IndexOf<T, Components...>();
        return std::get<ComponentStorage<T>>(mComponentStorages).mStorage[mComponentIndices[entity][typeIndex]];
    }
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mComponentStorages);
    }
    bool IsValidEntity(EntityID entity) const {
        return entity < mValidEntities.size()&& mValidEntities[entity];
    }
    const_iterator cbegin() const {
        return mEntities.cbegin();
    }
    const_iterator cend() const {
        return mEntities.cend();
    }

    const_iterator begin() {
        return mEntities.cbegin();
    }

    const_iterator end() {
        return mEntities.cend();
    }

    const_iterator begin() const {
        return mEntities.cbegin();
    }

    const_iterator end() const {
        return mEntities.cend();
    }

private:
    template <typename T, typename First, typename... Rest>
    static constexpr size_t IndexOf() {
        if constexpr (std::is_same_v<T, First>) {
            return 0;
        } else {
            static_assert( sizeof...(Rest) > 0,"Component type is not registered" );
            return 1 + IndexOf<T, Rest...>();
        }
    }
    tEntity mEntities;
    std::vector<bool> mValidEntities;
    std::vector<EntityID> mFreeEntities;
    std::vector<std::array<size_t, sizeof...(Components)>> mComponentIndices;
    std::tuple< ComponentStorage<Components>...> mComponentStorages;
};

}  // namespace ECSEngine
