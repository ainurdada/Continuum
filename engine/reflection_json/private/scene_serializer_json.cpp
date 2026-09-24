#include <scene_serializer_json.h>

#include <fstream>

#include <ecs_reflection/public/component_binding_registry.h>
#include <ecs_reflection/public/world_reflection_context.h>
#include <reflection/public/type_descriptor.h>
#include <reflection_json/public/json_serialization.h>
#include <reflection_json/public/json_serialization_registry.h>
#include <scene/public/parent.h>
#include <scene/public/scene_entity_id.h>

namespace engine::scene::serialization::json {

namespace {

struct ParsedEntity {
    SceneEntityId id;
    std::optional<SceneEntityId> parent;
    const nlohmann::json* componentsJson = nullptr;
};

struct SceneEntity {
    ecs::Entity entity;
    scene::SceneEntityId sceneId;
    std::optional<ecs::Entity> parent = std::nullopt;
};

std::expected<void, std::string> loadReflectedComponent(ecs::World& world, ecs::Entity entity, std::string_view key, const nlohmann::json& componentJson, const engine::reflection::TypeRegistry& typeReg, const engine::reflection::serialization::JsonSerializationRegistry& policies,
                                                        const engine::ecs_reflection::ComponentBindingRegistry& bindings) {
    auto desc = typeReg.findtype(key);
    if (!desc) {
        return std::unexpected("Unknown type: " + std::string(key));
    }

    auto binding = bindings.findBinding(desc->nativeTypeKey);
    if (!binding || !binding->create) {
        return std::unexpected("Not correct binding: " + std::string(key));
    }

    auto view = binding->create(world, entity, *desc);
    if (!view) {
        return std::unexpected(view.error() + ": " + std::string(key));
    }

    auto deserializationResult = reflection::serialization::deserializeValue(componentJson, *view, policies);
    if (!deserializationResult) {
        return std::unexpected(deserializationResult.error() + ": " + std::string(key));
    }

    return {};
}

std::expected<void, std::string> validateParentLinks(const std::unordered_set<std::uint64_t>& entityIds, const std::unordered_map<std::uint64_t, std::uint64_t>& childParentMap) {
    for (const auto& [childId, parentId] : childParentMap) {
        if (childId == parentId) {
            return std::unexpected("scene entity cannot be its own parent");
        }

        if (!entityIds.contains(parentId)) {
            return std::unexpected("parent scene entity is missing");
        }
    }

    for (const auto& link : childParentMap) {
        std::unordered_set<std::uint64_t> chainIds{};
        std::uint64_t currentId = link.first;

        while (childParentMap.contains(currentId)) {
            if (!chainIds.emplace(currentId).second) {
                return std::unexpected("parent hierarchy contains a cycle");
            }

            currentId = childParentMap.at(currentId);
        }
    }

    return {};
}

std::expected<SceneEntityId, std::string> jsonToSceneEntityId(const nlohmann::json& sceneEntityIdJson) {
    if (!sceneEntityIdJson.is_number_unsigned()) {
        return std::unexpected("scene entity id must be number");
    }
    std::uint64_t sceneEntityId = sceneEntityIdJson.get<std::uint64_t>();
    if (sceneEntityId <= 0) {
        return std::unexpected("not valid scene entity id");
    }
    return SceneEntityId{.value = sceneEntityId};
}

std::expected<nlohmann::json, std::string> reflectedComponentToJson(ecs::Entity entity, const engine::reflection::serialization::JsonSerializationRegistry& policies, ecs_reflection::WorldReflectionContext& ctx) {
    std::expected<nlohmann::json, std::string> result = nlohmann::json::object();

    if (!ctx.visitComponents(entity, [&result, &policies](const ecs::IStash& stash, std::optional<reflection::ObjectView> view) {
            if (!view || !result) {
                return;
            }
            auto serializedType = reflection::serialization::serializeValue(view.value(), policies);
            if (!serializedType) {
                result = std::unexpected(std::string(view->type()->key) + ": " + serializedType.error());
                return;
            }
            result->emplace(view->type()->key, serializedType.value());
        })) {
        return std::unexpected("Failed to visit components");
    }
    return result;
}

std::expected<std::vector<SceneEntity>, std::string> getSceneEntities(ecs::World& world) {
    auto& sceneIdStash = world.getStash<scene::SceneEntityId>();
    auto& parentStash = world.getStash<scene::Parent>();
    auto sceneIdQuery = world.query().with<scene::SceneEntityId>().build();
    std::unordered_set<std::uint64_t> visitedIds{};
    std::vector<SceneEntity> result{};
    for (auto entity : sceneIdQuery.view()) {
        scene::SceneEntityId sceneId = *sceneIdStash.get(entity);
        if (sceneId.value == 0) {
            return std::unexpected("not valid scene id");
        }
        if (!visitedIds.emplace(sceneId.value).second) {
            return std::unexpected("not unique scene id");
        }
        result.push_back(SceneEntity{.entity = entity, .sceneId = sceneId});
        Parent* parent = parentStash.getMut(entity);
        if (parent) {
            result.back().parent = parent->entity;
        }
    }
    std::sort(result.begin(), result.end(), [](const SceneEntity& a, const SceneEntity& b) { return a.sceneId.value < b.sceneId.value; });
    return result;
}

std::expected<SceneEntityId, std::string> getEntitySceneId(ecs::Entity entity, std::vector<SceneEntity> sceneEntities) {
    for (auto& sceneEntity : sceneEntities) {
        if (sceneEntity.entity == entity) {
            return sceneEntity.sceneId;
        }
    }

    return std::unexpected("Entity does not have scene id");
}

} // namespace

std::expected<nlohmann::json, std::string> SceneSerializerJson::worldToJson(ecs::World& world) {
    engine::ecs_reflection::WorldReflectionContext ctx{world, _bindings};
    auto sceneEntities = getSceneEntities(world);
    if (!sceneEntities.has_value()) {
        return std::unexpected(sceneEntities.error());
    }

    nlohmann::json sceneJson = nlohmann::json::object();
    sceneJson["formatVersion"] = 1;
    sceneJson["entities"] = nlohmann::json::array();

    const auto* sceneIdStash = world.findStash<scene::SceneEntityId>();

    std::unordered_set<std::uint64_t> entityIds;
    std::unordered_map<std::uint64_t, std::uint64_t> childParentMap;

    auto parentQuery = world.query().with<Parent>().with<SceneEntityId>().build();
    for (auto entity : parentQuery.view()) {
    }

    for (const SceneEntity& sceneEntity : sceneEntities.value()) {
        auto toJsonResult = reflectedComponentToJson(sceneEntity.entity, _policies, ctx);
        if (!toJsonResult) {
            return std::unexpected(toJsonResult.error());
        }
        auto componentsJson = toJsonResult.value();

        if (sceneEntity.parent) {
            auto parentSceneId = getEntitySceneId(sceneEntity.parent.value(), sceneEntities.value());
            if (!parentSceneId) {
                return std::unexpected(parentSceneId.error());
            }
            auto parentJson = nlohmann::json::object();
            parentJson["sceneId"] = parentSceneId->value;
            componentsJson["engine::scene::Parent"] = parentJson;
        }

        nlohmann::json entityJson = nlohmann::json::object();
        entityJson["id"] = sceneEntity.sceneId.value;
        entityJson["components"] = componentsJson;

        sceneJson["entities"].push_back(entityJson);

        if (!entityIds.emplace(sceneEntity.sceneId.value).second) {
            return std::unexpected("Failed to serialize scene: entities have not unique scene ids");
        }
        if (sceneEntity.parent) {
            auto parentSceneid = getEntitySceneId(sceneEntity.parent.value(), sceneEntities.value());
            if (!parentSceneid) {
                return std::unexpected(parentSceneid.error());
            }
            childParentMap.emplace(sceneEntity.sceneId.value, parentSceneid->value);
        }
    }

    auto validationResult = validateParentLinks(entityIds, childParentMap);
    if (!validationResult) {
        return std::unexpected(validationResult.error());
    }

    return sceneJson;
}

std::expected<void, std::string> SceneSerializerJson::jsonToWorld(ecs::World& world, const nlohmann::json& jsonScene) {
    // VALIDATION DATA BEGIN

    if (jsonScene.is_discarded()) {
        return std::unexpected<std::string>("not correct json file");
    }
    if (!jsonScene.is_object()) {
        return std::unexpected<std::string>("not correct json structure");
    }

    if (!jsonScene.contains("formatVersion")) {
        return std::unexpected<std::string>("missing formatVersion");
    }
    const auto& formatVersionJson = jsonScene.at("formatVersion");
    if (!formatVersionJson.is_number_integer()) {
        return std::unexpected<std::string>("formatVersion must be integer");
    }
    int formatVersion = formatVersionJson.get<int>();
    if (formatVersion != 1) {
        return std::unexpected<std::string>("required formatVersion: 1");
    }

    if (!jsonScene.contains("entities")) {
        return std::unexpected<std::string>("missing entities");
    }
    const auto& entitiesJson = jsonScene.at("entities");
    if (!entitiesJson.is_array()) {
        return std::unexpected<std::string>("entities must be array");
    }

    std::unordered_set<std::uint64_t> visitedIds{};
    std::vector<ParsedEntity> parsedEntities{};
    for (const auto& entity : entitiesJson) {
        if (!entity.is_object()) {
            return std::unexpected<std::string>("not correct json structure");
        }

        if (!entity.contains("id")) {
            return std::unexpected<std::string>("missing entity id");
        }
        const auto& entitySceneIdJson = entity.at("id");
        auto id = jsonToSceneEntityId(entitySceneIdJson);
        if (!id.has_value()) {
            return std::unexpected(id.error());
        }
        if (!visitedIds.emplace(id.value().value).second) {
            return std::unexpected<std::string>("entity id must be unique number");
        }

        if (!entity.contains("components")) {
            return std::unexpected<std::string>("missing entity components");
        }
        const auto& entityComponentsJson = entity.at("components");
        if (!entityComponentsJson.is_object()) {
            return std::unexpected<std::string>("not correct json structure");
        }

        ParsedEntity parsedEntity{.id = id.value(), .componentsJson = &entityComponentsJson};

        for (auto component : entityComponentsJson.items()) {
            if (component.key() == "engine::scene::Parent" && component.value().contains("sceneId")) {
                parsedEntity.parent = SceneEntityId{component.value().at("sceneId").get<std::uint64_t>()};
            }
        }

        parsedEntities.push_back(parsedEntity);
    }

    // check cycles in parent links
    std::unordered_map<std::uint64_t, std::uint64_t> childParentMap{};
    for (const auto& entity : parsedEntities) {
        if (entity.parent) {
            childParentMap.emplace(entity.id.value, entity.parent.value().value);
        }
    }
    const auto parentValidationResult = validateParentLinks(visitedIds, childParentMap);
    if (!parentValidationResult.has_value()) {
        return std::unexpected(parentValidationResult.error());
    }

    // VALIDATION DATA END

    // SET WORLD BEGIN

    std::vector<ecs::Entity> createdEntites{};
    std::unordered_map<std::uint64_t, ecs::Entity> entitySceneIdMap{};
    for (auto& parsedEntity : parsedEntities) {
        auto entity = world.createEntity();
        world.getStash<scene::SceneEntityId>().add(entity, {parsedEntity.id});
        createdEntites.push_back(entity);
        entitySceneIdMap.emplace(parsedEntity.id.value, entity);
        for (auto& [key, componentJson] : parsedEntity.componentsJson->items()) {
            if (key == "engine::scene::Parent") {
                continue;
            }
            auto component = loadReflectedComponent(world, entity, key, componentJson, _typeReg, _policies, _bindings);
            if (!component) {
                for (auto created : createdEntites) {
                    world.destroyEntity(created);
                }
                return std::unexpected("Failed to create component " + key + ": " + component.error());
            }
        }
    }

    auto& parentStash = world.getStash<scene::Parent>();
    for (auto& parsedEntity : parsedEntities) {
        if (parsedEntity.parent) {
            auto child = entitySceneIdMap.at(parsedEntity.id.value);
            auto parent = entitySceneIdMap.at(parsedEntity.parent.value().value);
            parentStash.add(child, scene::Parent{.entity = parent});
        }
    }

    // SET WORLD END

    return {};
}

std::expected<void, std::string> SceneSerializerJson::serialize(ecs::World& world, std::filesystem::path sceneFile) {
    if (sceneFile.extension() != ".cscn") {
        return std::unexpected("not valid scene file format");
    }

    engine::ecs_reflection::WorldReflectionContext ctx{world, _bindings};
    auto worldJson = worldToJson(world);
    if (!worldJson.has_value()) {
        return std::unexpected(worldJson.error());
    }

    std::string sceneString;
    try {
        sceneString = worldJson.value().dump(4);
    } catch (const nlohmann::json::exception& exception) {
        return std::unexpected(std::string{"failed to convert scene JSON to text: "} + exception.what());
    }

    std::ofstream sceneStream{sceneFile, std::ios::binary | std::ios::trunc};

    if (!sceneStream.is_open()) {
        return std::unexpected("could not open scene file for writing");
    }

    sceneStream << sceneString << '\n';
    sceneStream.close();

    if (sceneStream.fail()) {
        return std::unexpected("could not write scene file");
    }

    return {};
}

std::expected<void, std::string> SceneSerializerJson::deserialize(ecs::World& world, std::filesystem::path sceneFile) {
    // VALIDATION DATA BEGIN

    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(sceneFile, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path");
    }
    errorCode.clear();
    bool isRegularFile = std::filesystem::is_regular_file(canonicalPath, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>(errorCode.message());
    }
    if (!isRegularFile) {
        return std::unexpected<std::string>("scene file is not regular file");
    }
    if (canonicalPath.extension() != ".cscn") {
        return std::unexpected<std::string>("missing continuum scene extension");
    }

    std::ifstream sceneStream{canonicalPath};
    if (!sceneStream.is_open()) {
        return std::unexpected<std::string>("could not open scene file");
    }

    // VALIDATION DATA END

    auto jsonScene = nlohmann::json::parse(sceneStream, nullptr, false);

    auto worldParsingResult = jsonToWorld(world, jsonScene);
    if (!worldParsingResult) {
        return std::unexpected(worldParsingResult.error());
    }

    return {};
}

} // namespace engine::scene::serialization::json
