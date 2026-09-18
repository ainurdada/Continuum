#pragma once

#include <cstddef>
#include <typeindex>

#include <public/archetype.h>
#include <public/query.h>
#include <public/execution_layer.h>
#include "public/stash.h"

namespace engine::ecs {

// Basic ECS world
class World {
  private:
    enum class SlotState {
        Free,
        Reserved,
        Alive
    };

    struct WorldSlot {
        std::size_t rowInArchetype{};
        ArchetypeID archetypeId{};
        std::uint32_t generation{};
        SlotState state = SlotState::Free;
    };

    class IterationScope {
        std::size_t& _counter;

      public:
        IterationScope(std::size_t& counter);
        ~IterationScope() noexcept;

        IterationScope(const IterationScope&) = delete;
        IterationScope& operator=(const IterationScope&) = delete;
        IterationScope(const IterationScope&&) = delete;
        IterationScope& operator=(IterationScope&&) = delete;
    };

    friend QueryView;
    friend QueryBuilder;
    friend ExecutionLayer;
    friend WorldExecution;
    friend Commands;

    mutable std::size_t _activeIterationCount{};

    std::vector<WorldSlot> _slots{};
    std::vector<std::uint32_t> _freeList{};

    std::unordered_map<std::type_index, ComponentID> _componentRegistryMap;
    std::vector<std::unique_ptr<IStash>> _stashes{};

    ArchetypeRegistry _archetypeRegistry{};
    QueryCache _queryCache{};

    Commands* _commands{};

    bool _isCommitState = false;

    ExecutionLayer _executionLayer;

    template <typename T> friend struct Stash;
    template <typename T> void addEntityComponent(Entity entity, Stash<T>& stash, const T& component);
    template <typename T> void removeEntityComponent(Entity entity, Stash<T>& stash);
    void setEntityArchetype(Entity entity, ArchetypeID archetypeID);
    void validateNoActiveIteration();
    void validateCommitState() const;
    const QueryCacheEntry& getQueryEntry(QueryID id) const;
    Query createQuery(const QueryFilter& filter);
    bool destroyEntityImmediate(Entity entity);
    void applyCommandBuffer(const CommandBuffer& commandBuffer);
    bool removeEntityComponentImmediate(Entity entity, ComponentID componentId);
    void setEntityArchetypePrepared(Entity entity, ArchetypeID desiredArchetypeId) noexcept;
    Entity reserveEntity();
    void prepareFreeListForNewSlot();
    bool releaseReservedEntity(Entity entity) noexcept;
    void releaseReservations(const CommandBuffer& commandBuffer) noexcept;
    bool hasReservedEntity(Entity entity) const noexcept;
    void commitReservedEntityPrepared(Entity entity, ArchetypeID archetypeId) noexcept;

  public:
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    World() : _executionLayer{*this} {}

    Entity createEntity();
    bool hasEntity(Entity entity) const;
    void destroyEntity(Entity entity);

    template <typename T> Stash<T>& getStash();
    template <typename T> const Stash<T>* findStash() const;
    const IStash* findStash(ComponentID id) const noexcept;
    std::size_t stashCount() const noexcept;

    /// @brief Makes visitor to visit all entity component stashes
    /// @param visitor example: void visitor(const IStash&)
    /// @return Was visiting successful
    template <typename Visitor> bool visitComponents(Entity entity, Visitor&& visitor) const;

    /// @brief Makes visitor to visit all entity component stashes
    /// @param visitor example: void visitor(IStash&)
    /// @return Was visiting successful
    template <typename Visitor> bool visitComponentsMut(Entity entity, Visitor&& visitor);

    QueryBuilder query();

    void commit();

    SystemGroup& createSystemGroup();
};

}