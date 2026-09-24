#include <emission/ecs_emitter.h>

#include <algorithm>

namespace {

std::string emitComponentRegistration(const TypeModel& typeModel) {
    std::string command = "";
    if (std::find_if(typeModel.modifiers.begin(), typeModel.modifiers.end(), [](const ModifierModel& mod) { return mod.qualifiedTypeName == "::engine::reflection::modifiers::Component"; }) == typeModel.modifiers.end()) {
        return command;
    }
    command += "if (!bindings.registerComponent<::" + typeModel.name + ">(typeRegistry)) {\n";
    command += "    return false;\n";
    command += "}\n";
    return command;
}

} // namespace

std::string emitEcsBindingSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName) {
    std::string command = "";
    for (auto& include : includePaths) {
        command += "#include \"" + include + "\"\n";
    }
    command += "#include <engine/ecs_reflection/public/component_binding_registry.h>\n";
    command += "#include <engine/reflection/public/type_registry.h>\n";

    command += "namespace engine::ecs_reflection::generated::module_" + moduleName + " {\n\n";

    command += "bool registerComponent(engine::ecs_reflection::ComponentBindingRegistry& bindings, const engine::reflection::TypeRegistry& typeRegistry) {\n";
    for (const auto& model : typeModels) {
        command += emitComponentRegistration(model);
    }
    command += "return true;\n";
    command += "}\n\n";

    command += "}\n";
    return command;
}

std::string emitEcsBootstrapSource(const std::vector<std::string>& moduleNames) {
    std::string command = "";
    command += "#include <engine/ecs_reflection/public/component_binding_registry.h>\n";
    command += "#include <engine/ecs_reflection/public/ecs_reflection_bootstrap.h>\n";
    command += "#include <engine/reflection/public/type_registry.h>\n";

    for (auto& moduleName : moduleNames) {
        command += "namespace engine::ecs_reflection::generated::module_" + moduleName + " {\n";
        command += "bool registerComponent(engine::ecs_reflection::ComponentBindingRegistry& bindings, const engine::reflection::TypeRegistry& typeRegistry);\n";
        command += "}\n\n";
    }

    command += "namespace engine::ecs_reflection::generated {\n\n";
    command += "bool initializeComponentBindings(engine::ecs_reflection::ComponentBindingRegistry& bindings, const engine::reflection::TypeRegistry& typeRegistry) {\n";
    command += "    if (bindings.isFrozen() || !typeRegistry.isFrozen()) {\n";
    command += "        return false;\n";
    command += "    }\n\n";
    for (auto& moduleName : moduleNames) {
        command += "    if (!engine::ecs_reflection::generated::module_" + moduleName + "::registerComponent(bindings, typeRegistry)) {\n";
        command += "        return false;\n";
        command += "    }\n\n";
    }
    command += "    bindings.freeze();\n";
    command += "    return true;\n";
    command += "}\n\n";
    command += "}\n";

    return command;
}
