#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <component_signature.h>
#include <entity.h>

namespace engine::ecs {

class World;
class ExecutionLayer;

class ICommandPayloadStorage {
  public:
    virtual ~ICommandPayloadStorage() = default;
    virtual void clear() noexcept = 0;
    virtual void* getValue(std::size_t index) noexcept = 0;
    virtual std::size_t size() const noexcept = 0;
};

template <typename T> struct CommandPayloadStorage : public ICommandPayloadStorage {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

  private:
    std::vector<T> _values;

  public:
    std::size_t add(T&& value);
    void pop() noexcept;
    void* getValue(std::size_t index) noexcept override;
    std::size_t size() const noexcept override;
    void clear() noexcept override;
};

class CommandBuffer {
  private:
    friend ExecutionLayer;
    friend World;
    friend struct Commands;

    enum class CommandKind {
        DestroyEntity,
        RemoveEntityComponent,
        AddEntityComponent,
        CreateEntity
    };

    struct CommandData {
        CommandKind commandKind;
        Entity entity;
        ComponentID componentId{};
        std::size_t payloadIndex{};
    };

    std::vector<CommandData> _requestedCommands{};
    std::vector<std::unique_ptr<ICommandPayloadStorage>> _commandPayloadStorages{};

    CommandBuffer() = default;

    CommandBuffer(const CommandBuffer& other) = delete;
    CommandBuffer& operator=(const CommandBuffer& other) = delete;

    void requestCommand(CommandData data);
    void clear() noexcept;
    template <typename T> CommandPayloadStorage<T>& getOrCreateCommandPayloadStorage(ComponentID componentId);

  public:
    CommandBuffer(CommandBuffer&& other) = default;
    CommandBuffer& operator=(CommandBuffer&& other) = default;
};

struct Commands {
  private:
    friend ExecutionLayer;
    friend World;

    World& _world;
    CommandBuffer& _commandBuffer;

    Commands() = delete;

    Commands(const Commands&) = delete;
    Commands& operator=(const Commands&) = delete;
    Commands(Commands&&) = delete;
    Commands& operator=(Commands&&) = delete;

    Commands(World& world, CommandBuffer& commandBuffer) : _world(world), _commandBuffer(commandBuffer) {}

  public:
    void destroy(Entity entity);
    template <typename T> void remove(Entity entity);
    template <typename T> void add(Entity entity, T component);
    Entity create();
};

template <typename T> std::size_t CommandPayloadStorage<T>::add(T&& value) {
    std::size_t newIndex = _values.size();
    _values.push_back(std::move(value));
    return newIndex;
}

template <typename T> inline void CommandPayloadStorage<T>::pop() noexcept {
    assert(_values.size() != 0);
    _values.pop_back();
}

template <typename T> inline void* CommandPayloadStorage<T>::getValue(std::size_t index) noexcept {
    assert(index < _values.size());
    return &_values[index];
}

template <typename T> inline std::size_t CommandPayloadStorage<T>::size() const noexcept {
    return _values.size();
}

template <typename T> inline void CommandPayloadStorage<T>::clear() noexcept {
    _values.clear();
}

template <typename T> CommandPayloadStorage<T>& CommandBuffer::getOrCreateCommandPayloadStorage(ComponentID componentId) {
    std::size_t index = static_cast<std::size_t>(componentId);
    if (index >= _commandPayloadStorages.size()) {
        _commandPayloadStorages.resize(index + 1);
    }
    if (!_commandPayloadStorages[index]) {
        _commandPayloadStorages[index] = std::make_unique<CommandPayloadStorage<T>>();
    }
    return *static_cast<CommandPayloadStorage<T>*>(_commandPayloadStorages[index].get());
}

} // namespace engine::ecs