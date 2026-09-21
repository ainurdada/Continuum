#pragma once

#include <ecs/ecs.h>
#include <expected>
#include <filesystem>
#include <memory>
#include <string>

namespace engine::scene::serialization {
class ISceneSerializer;
}

namespace editor {

class SceneDocument {
  private:
    engine::ecs::World _world{};
    std::filesystem::path _scenePath;
    bool _isDirty{};

    SceneDocument() = default;

  public:
    /// @brief Create and load scene
    /// @param scenePath Path to scene file
    /// @return SceneDocument unique ptr if has value. Error description if does not have value
    [[nodiscard]] static std::expected<std::unique_ptr<SceneDocument>, std::string> open(const std::filesystem::path& scenePath, engine::scene::serialization::ISceneSerializer& serializer);

    /// @brief Get world
    /// @return Constant world
    [[nodiscard]] const engine::ecs::World& world() const noexcept;

    /// @brief Get mutable world
    /// @return Mutable world
    engine::ecs::World& worldMut();

    /// @brief Get scene path
    /// @return Path to scene file
    [[nodiscard]] const std::filesystem::path& path() const noexcept;

    /// @brief Scene is dirty after any changes in world (i.e. moving object or changing its name)
    /// @return Was scene changed and was not saved
    [[nodiscard]] bool isDirty() const noexcept;

    /// @brief Mark scene as dirty
    void markDirty();

    /// @brief Save scene. If saving is successful it removes dirty flag
    /// @return Error description if does not have value
    std::expected<void, std::string> save(engine::scene::serialization::ISceneSerializer& serializer);

    /// @brief Create new empty entity
    /// @return Created entity if has value. Error description if does not have value
    std::expected<engine::ecs::Entity, std::string> createEmptyEntity();

    /// @brief Delete entity
    /// @param deleteChildren If true then also delete all children
    /// @return Error description if does not have value
    std::expected<void, std::string> deleteEntity(engine::ecs::Entity entity, bool deleteChildren = true);
};

} // namespace editor
