#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>

#include "component_signature.h"
#include "entity.h"

namespace engine::ecs {

using ArchetypeID = std::uint32_t;

struct Archetype {
  private:
    ArchetypeID _id;
    ComponentSignature _signature;
    std::vector<Entity> _denseEntities{};

  public:
    Archetype(ArchetypeID id, const ComponentSignature& signature) : _id(id), _signature(signature) {}
    ArchetypeID id() const noexcept;
    const ComponentSignature& signature() const noexcept;
    std::size_t size() const;
    const std::vector<Entity>& entities() const;
    std::size_t addEntity(Entity entity);
    std::optional<Entity> removeEntity(std::size_t row) noexcept;

    void prepareAddEntity();
    std::size_t addPreparedEntity(Entity entity) noexcept;
};

struct ArchetypeLookupResult {
    ArchetypeID id{};
    bool created{};
};

struct QueryFilter;

struct ArchetypeRegistry {
  private:
    std::vector<Archetype> _archetypes{};
    std::unordered_map<ComponentSignature, ArchetypeID, ComponentSignatureHash> _archetypeIdMap{};

  public:
    ArchetypeRegistry();
    ArchetypeLookupResult getOrCreate(const ComponentSignature& signature);
    Archetype& get(ArchetypeID id);
    const Archetype& get(ArchetypeID id) const;
    std::size_t size() const noexcept;
    std::vector<ArchetypeID> filter(const QueryFilter& filter) const;
};

} // namespace engine::ecs