#pragma once

namespace engine::reflection {

class TypeRegistry;

} // namespace engine::reflection

namespace engine::ecs_reflection {

class ComponentBindingRegistry;

namespace generated {

bool initializeComponentBindings(ComponentBindingRegistry& bindings, const engine::reflection::TypeRegistry& typeRegistry);

} // namespace generated

} // namespace engine::ecs_reflection
