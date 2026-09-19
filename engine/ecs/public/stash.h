#pragma once

#include <typeindex>

#include "component_signature.h"
#include "entity.h"

namespace engine::ecs {

class World;

class IStash {
  public:
    virtual ComponentID componentId() const noexcept = 0;
    virtual std::type_index nativeTypeKey() const noexcept = 0;
    virtual ~IStash() = default;

  private:
    friend World;
    virtual bool removeStorage(Entity entity) noexcept = 0;
    virtual void prepareAddStorage(Entity entity) = 0;
    virtual void addPreparedStorage(Entity entity, void* component) noexcept = 0;
};

template <typename T> struct Stash : public IStash {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

  private:
    friend World;

    World& _world;
    ComponentID _componentId;
    std::vector<Entity> _denseEntities{};
    std::vector<T> _denseComponents{};
    std::vector<std::size_t> _sparse{};
    const std::size_t invalidDenseIndex = std::numeric_limits<std::size_t>::max();

    Stash(const Stash&) = delete;
    Stash& operator=(const Stash&) = delete;

    Stash() = delete;
    Stash(World& world, ComponentID id) : _world(world), _componentId(id) {}

    std::size_t findDenseIndex(Entity entity) const;
    bool addStorage(Entity entity, const T& component);
    bool removeStorage(Entity entity) noexcept override;
    void prepareAddStorage(Entity entity) override;
    void addPreparedStorage(Entity entity, void* component) noexcept override;

  public:
    ComponentID componentId() const noexcept override;
    std::type_index nativeTypeKey() const noexcept override;
    bool has(Entity entity) const;
    void add(Entity entity, const T& component);
    const T* get(Entity entity) const;
    T* getMut(Entity entity);
    void remove(Entity entity);
    std::size_t size() const noexcept;
};

} // namespace engine::ecs