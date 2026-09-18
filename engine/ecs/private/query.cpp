#include <public/query.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

#include <public/world.h>

namespace engine::ecs {

bool QueryFilter::require(ComponentID id) {
    return !_none.has(id) && _all.add(id);
}

bool QueryFilter::exclude(ComponentID id) {
    return !_all.has(id) && _none.add(id);
}

const ComponentSignature& QueryFilter::all() const noexcept {
    return _all;
}

const ComponentSignature& QueryFilter::none() const noexcept {
    return _none;
}

bool QueryFilter::matches(const ComponentSignature& signature) const {
    return signature.containsAll(_all) && !signature.intersects(_none);
}

std::size_t QueryFilterHash::operator()(const QueryFilter& filter) const noexcept {
    ComponentSignatureHash hashFunctor;
    std::size_t hashAll = hashFunctor(filter.all());
    std::size_t hashNone = hashFunctor(filter.none());
    return hashAll + std::size_t{0x9e3779b9U} + (hashAll << 6U) + (hashNone >> 2U) + hashNone;
}

QueryCacheEntry::QueryCacheEntry(const ArchetypeRegistry& registry, const QueryFilter& filter) {
    _filter = filter;
    _ids = registry.filter(filter);
}

const QueryFilter& QueryCacheEntry::filter() const noexcept {
    return _filter;
}

const std::vector<ArchetypeID>& QueryCacheEntry::ids() const noexcept {
    return _ids;
}

bool QueryCacheEntry::tryAddArchetype(const Archetype& archetype) {
    if (!_filter.matches(archetype.signature())) {
        return false;
    }

    auto it = std::lower_bound(_ids.begin(), _ids.end(), archetype.id());
    if (it != _ids.end() && *it == archetype.id()) {
        return false;
    }

    _ids.insert(it, archetype.id());
    return true;
}

void QueryCacheEntry::prepareAddArchetype(const ComponentSignature& signature) {
    if (_filter.matches(signature)) {
        if (_ids.size() == _ids.capacity()) {
            auto newCapacity = 2 * _ids.capacity();
            if (newCapacity == 0) {
                newCapacity = 1;
            }
            _ids.reserve(newCapacity);
        }
    }
}

QueryID QueryCache::getOrCreateEntry(const ArchetypeRegistry& registry, const QueryFilter& filter) {
    if (_queryMap.contains(filter)) {
        return _queryMap.at(filter);
    }
    if (_entries.size() == static_cast<std::size_t>(std::numeric_limits<QueryID>::max()) + 1) {
        throw std::length_error("QueryCacheEntry count limit");
    }
    QueryCacheEntry newEntry = QueryCacheEntry(registry, filter);
    _entries.push_back(newEntry);
    QueryID result = static_cast<QueryID>(_entries.size() - 1);
    try {
        auto [it, emplaced] = _queryMap.emplace(filter, result);
        if (!emplaced) {
            _entries.pop_back();
            return it->second;
        }
    } catch (...) {
        _entries.pop_back();
        throw;
    }
    return result;
}

const QueryCacheEntry& QueryCache::getEntry(QueryID id) const {
    return _entries.at(id);
}

std::size_t QueryCache::size() const noexcept {
    return _entries.size();
}

void QueryCache::onArchetypeCreated(const Archetype& archetype) {
    for (auto& entry : _entries) {
        entry.tryAddArchetype(archetype);
    }
}

void QueryCache::prepareAddArchetype(const ComponentSignature& signature) {
    for (auto& entry : _entries) {
        entry.prepareAddArchetype(signature);
    }
}

QueryView Query::view() const {
    return QueryView(_world, _queryId);
}

QueryView::QueryView(World* world, QueryID id) {
    _world = world;
    _registry = &_world->_archetypeRegistry;
    _entry = &_world->getQueryEntry(id);
    _world->_activeIterationCount++;
}

QueryView::Iterator QueryView::begin() const {
    QueryView::Iterator it{};
    it._entry = _entry;
    it._registry = _registry;
    it._archetypeIndex = 0;
    it._entityIndex = 0;
    it.normalize();
    return it;
}

QueryView::Iterator QueryView::end() const {
    QueryView::Iterator it{};
    it._entry = _entry;
    it._registry = _registry;
    it._denseEntities = nullptr;
    it._archetypeIndex = _entry->ids().size();
    it._entityIndex = 0;
    return it;
}

QueryView::~QueryView() noexcept {
    assert(_world->_activeIterationCount > 0);
    _world->_activeIterationCount--;
}

void QueryView::Iterator::normalize() {
    while (_archetypeIndex < _entry->ids().size()) {
        _denseEntities = &_registry->get(_entry->ids()[_archetypeIndex]).entities();
        if (_entityIndex < _denseEntities->size()) {
            return;
        }

        _archetypeIndex++;
        _entityIndex = 0;
    }
    _denseEntities = nullptr;
}

Entity QueryView::Iterator::operator*() const {
    return _denseEntities->at(_entityIndex);
}

QueryView::Iterator& QueryView::Iterator::operator++() {
    if (++_entityIndex < _denseEntities->size()) {
        return *this;
    }
    normalize();
    return *this;
}

bool QueryView::Iterator::operator==(const Iterator& other) const {
    return _entry == other._entry && _archetypeIndex == other._archetypeIndex && _entityIndex == other._entityIndex;
}

Query QueryBuilder::build() {
    return _world->createQuery(_filter);
}

} // namespace engine::ecs
