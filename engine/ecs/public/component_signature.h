#pragma once

#include <cstdint>
#include <vector>


namespace engine::ecs {

using ComponentID = std::uint32_t;

struct ComponentSignature {
  private:
    std::vector<ComponentID> _signature{};

  public:
    ComponentSignature() = default;
    ComponentSignature(std::initializer_list<ComponentID> signature);
    bool has(ComponentID id) const;
    bool add(ComponentID id);
    bool remove(ComponentID id);
    std::size_t size() const;
    const std::vector<ComponentID>& get() const;
    bool operator==(const ComponentSignature& other) const;
    bool containsAll(const ComponentSignature& required) const;
    bool intersects(const ComponentSignature& other) const;
};

struct ComponentSignatureHash {
    std::size_t operator()(const ComponentSignature& signature) const noexcept;
};

}
