#include <cassert>
#include <stdexcept>

#include <public/archetype.h>
#include <public/query.h>

namespace engine::ecs {
ArchetypeID Archetype::id() const noexcept {
    return _id;
}

const ComponentSignature& Archetype::signature() const noexcept {
    return _signature;
}

std::size_t Archetype::size() const {
    return _denseEntities.size();
}

const std::vector<Entity>& Archetype::entities() const {
    return _denseEntities;
}

std::size_t Archetype::addEntity(Entity entity) {
    prepareAddEntity();
    return addPreparedEntity(entity);
}

std::optional<Entity> Archetype::removeEntity(std::size_t row) noexcept {
    assert(row < size());

    std::size_t lastIndex = size() - 1;
    if (row == lastIndex) {
        _denseEntities.pop_back();
    } else {
        Entity lastEntity = _denseEntities[lastIndex];
        _denseEntities[row] = lastEntity;
        _denseEntities.pop_back();
        return lastEntity;
    }
    return std::nullopt;
}

void Archetype::prepareAddEntity() {
    if (_denseEntities.size() == _denseEntities.capacity()) {
        auto newCapacity = 2 * _denseEntities.capacity();
        if (newCapacity == 0) {
            newCapacity = 1;
        }
        _denseEntities.reserve(newCapacity);
    }
}

std::size_t Archetype::addPreparedEntity(Entity entity) noexcept {
    assert(_denseEntities.size() < _denseEntities.capacity());

    auto newIndex = _denseEntities.size();
    _denseEntities.push_back(entity);
    return newIndex;
}

ArchetypeRegistry::ArchetypeRegistry() {
    _archetypes.push_back(Archetype{0, {}});
    _archetypeIdMap[{}] = 0;
}

ArchetypeLookupResult ArchetypeRegistry::getOrCreate(const ComponentSignature& signature) {
    if (_archetypeIdMap.contains(signature)) {
        return {.id = _archetypeIdMap.at(signature), .created = false};
    }

    if (_archetypes.size() > std::numeric_limits<ArchetypeID>::max()) {
        throw std::length_error("archetypes count more than ArchetypeID max available value");
    }
    ArchetypeID newId = static_cast<ArchetypeID>(_archetypes.size());
    _archetypes.push_back(Archetype{newId, signature});
    try {
        _archetypeIdMap.emplace(signature, newId);
    } catch (...) {
        _archetypes.pop_back();
        throw;
    }

    return {.id = newId, .created = true};
}

Archetype& ArchetypeRegistry::get(ArchetypeID id) {
    return _archetypes.at(id);
}

const Archetype& ArchetypeRegistry::get(ArchetypeID id) const {
    return _archetypes.at(id);
}

std::size_t ArchetypeRegistry::size() const noexcept {
    return _archetypes.size();
}

std::vector<ArchetypeID> ArchetypeRegistry::filter(const QueryFilter& filter) const {
    std::vector<ArchetypeID> result{};
    result.reserve(_archetypes.size());
    for (const auto& archetype : _archetypes) {
        if (filter.matches(archetype.signature())) {
            result.push_back(archetype.id());
        }
    }
    return result;
}

} // namespace engine::ecs
