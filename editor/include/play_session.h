#pragma once

#include <expected>
#include <string>

#include <nlohmann/json.hpp>

#include <ecs/ecs.h>
#include <ecs_reflection/public/component_binding_registry.h>
#include <reflection/public/type_registry.h>
#include <reflection_json/public/json_serialization_registry.h>
#include <render/public/render_frame_data.h>
#include <scene/scene.h>

namespace editor {

class PlaySession {
    engine::ecs::World _world{};
    engine::ecs::WorldExecution _worldExecution{_world};
    engine::Scene _playScene{_world};

  public:
    std::expected<void, std::string> run(const nlohmann::json& sceneJson, engine::reflection::TypeRegistry& typeReg, engine::reflection::serialization::JsonSerializationRegistry& policies, engine::ecs_reflection::ComponentBindingRegistry& bindings);

    void update(float deltaTime);
    void renderScene(engine::RenderFrameData& frame);
    void destroy();

    ~PlaySession();
};

} // namespace editor