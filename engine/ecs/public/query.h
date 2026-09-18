#pragma once

#include <cstdint>

#include <public/component_signature.h>
#include <public/archetype.h>

namespace engine::ecs {

class World;

using QueryID = std::uint32_t;

struct QueryFilter {
  private:
    ComponentSignature _all{};
    ComponentSignature _none{};

  public:
    bool require(ComponentID id);
    bool exclude(ComponentID id);
    const ComponentSignature& all() const noexcept;
    const ComponentSignature& none() const noexcept;
    bool matches(const ComponentSignature& signature) const;
    bool operator==(const QueryFilter& other) const = default;
};

struct QueryFilterHash {
    std::size_t operator()(const QueryFilter& filter) const noexcept;
};

struct QueryCacheEntry {
  private:
    QueryFilter _filter{};
    std::vector<ArchetypeID> _ids;

  public:
    QueryCacheEntry(const ArchetypeRegistry& registry, const QueryFilter& filter);
    const QueryFilter& filter() const noexcept;
    const std::vector<ArchetypeID>& ids() const noexcept;
    bool tryAddArchetype(const Archetype& archetype);
    void prepareAddArchetype(const ComponentSignature& signature);
};

struct QueryCache {
  private:
    std::vector<QueryCacheEntry> _entries{};
    std::unordered_map<QueryFilter, QueryID, QueryFilterHash> _queryMap;

  public:
    QueryID getOrCreateEntry(const ArchetypeRegistry& registry, const QueryFilter& filter);
    const QueryCacheEntry& getEntry(QueryID id) const;
    std::size_t size() const noexcept;
    void onArchetypeCreated(const Archetype& archetype);
    void prepareAddArchetype(const ComponentSignature& signature);
};

struct QueryView;

struct Query {
  private:
    friend World;
    World* _world;
    QueryID _queryId;

    Query() = delete;
    Query(World* world, QueryID queryId) : _world(world), _queryId(queryId) {};

  public:
    bool operator==(const Query& other) const = default;
    QueryView view() const;
};

struct QueryView {
  private:
    friend Query;
    World* _world;
    const QueryCacheEntry* _entry;
    const ArchetypeRegistry* _registry;

    QueryView() = delete;
    QueryView(World* world, QueryID id);

    QueryView(const QueryView& other) = delete;
    QueryView& operator=(const QueryView& other) = delete;

    QueryView(QueryView&& other) = delete;
    QueryView& operator=(QueryView&& other) = delete;

  public:
    struct Iterator {
      private:
        friend QueryView;
        const QueryCacheEntry* _entry{};
        const ArchetypeRegistry* _registry{};
        std::vector<Entity> const* _denseEntities{};
        std::size_t _archetypeIndex{};
        std::size_t _entityIndex{};

        void normalize();

      public:
        Entity operator*() const;
        Iterator& operator++();
        bool operator==(const Iterator& other) const;
    };

    Iterator begin() const;
    Iterator end() const;

    ~QueryView() noexcept;
};

class QueryBuilder {
  private:
    friend World;
    World* _world;
    QueryFilter _filter{};

    QueryBuilder() = delete;
    QueryBuilder(World& world) : _world(&world) {}

  public:
    template <typename T> QueryBuilder& with();
    template <typename T> QueryBuilder& without();
    Query build();
};

}