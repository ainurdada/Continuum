#include "emitter.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <variant>
#include <vector>
#include <unordered_map>
#include <span>

const std::unordered_map<BuiltinKind, std::string> builtinKindEmitMap{
    {BuiltinKind::Void, "engine::reflection::voidType()"},   {BuiltinKind::Bool, "engine::reflection::boolType()"},     {BuiltinKind::Int, "engine::reflection::intType()"},
    {BuiltinKind::Float, "engine::reflection::floatType()"}, {BuiltinKind::Double, "engine::reflection::doubleType()"}, {BuiltinKind::String, "engine::reflection::stringType()"},
};

const std::map<Qualifier, std::string> qualifiersEmitMap{
    {Qualifier::None, "engine::reflection::TypeQualifier::None"},
    {Qualifier::Const, "engine::reflection::TypeQualifier::Const"},
    {Qualifier::Volatile, "engine::reflection::TypeQualifier::Volatile"},
};

namespace {

std::string getFieldReaderName(const FieldModel& field, std::size_t typeIndex, std::size_t fieldIndex) {
    std::string command = "";
    if (hasAny(field.type.qualifiers, Qualifier::Volatile)) {
        return command;
    }
    command += "read_field_" + std::to_string(typeIndex) + "_" + std::to_string(fieldIndex);
    return command;
}

std::string getFieldMutableAccessorName(const FieldModel& field, std::size_t typeIndex, std::size_t fieldIndex) {
    std::string command = "";
    if (hasAny(field.type.qualifiers, Qualifier::Volatile | Qualifier::Const)) {
        return command;
    }
    command += "mutable_field_" + std::to_string(typeIndex) + "_" + std::to_string(fieldIndex);
    return command;
}

std::string emitKeyExpression(const std::vector<ModifierModel>& mods, const std::string& prefix, std::string defaultKey) {
    for (std::size_t i = 0; i < mods.size(); i++) {
        if (mods[i].qualifiedTypeName == "::engine::reflection::modifiers::Key") {
            return prefix + "_value_" + std::to_string(i) + ".value";
        }
    }
    return "\"" + defaultKey + "\"";
}

std::string emitNativeTypeKey(const TypeRefModel& ref) {
    if (std::holds_alternative<BuiltinKind>(ref.target)) {
        auto builtinKind = std::get<BuiltinKind>(ref.target);
        if (!builtinKindEmitMap.contains(builtinKind)) {
            throw std::logic_error("No builtin kind in map");
        }
        return builtinKindEmitMap.at(builtinKind) + "->nativeTypeKey";
    }
    if (std::holds_alternative<RecordTypeRefModel>(ref.target)) {
        return "typeid(::" + std::get<RecordTypeRefModel>(ref.target).qualifiedName + ")";
    }
    throw std::runtime_error("Uknown type");
}

std::string emitFieldDescriptor(const FieldModel& model, std::string readAddress, std::string mutableAddress, const std::string& prefix) {
    std::string command = "engine::reflection::FieldDescriptor{\n";
    command += ".nativeTypeKey = " + emitNativeTypeKey(model.type) + ",\n";
    command += ".key = " + emitKeyExpression(model.modifiers, prefix, model.name) + ",\n";
    command += ".name = \"" + model.name + "\",\n";

    command += ".modifiers = ::engine::reflection::Modifiers(" + prefix + "_entries),\n";

    command += ".qualifiers = ";
    if (model.type.qualifiers == Qualifier::None) {
        command += qualifiersEmitMap.at(Qualifier::None);
    } else {
        std::vector<std::string> qualifiers{};
        for (auto [q, s] : qualifiersEmitMap) {
            if ((q & model.type.qualifiers) != Qualifier::None) {
                qualifiers.push_back(s);
            }
        }
        for (std::uint8_t i = 0; i < qualifiers.size(); i++) {
            command += qualifiers[i];
            if (i != qualifiers.size() - 1) {
                command += " | ";
            }
        }
    }
    command += ",\n";

    if (!readAddress.empty()) {
        command += ".readAddress = " + readAddress + ",\n";
    } else {
        command += ".readAddress = nullptr,\n";
    }

    if (!mutableAddress.empty()) {
        command += ".mutableAddress = " + mutableAddress + ",\n";
    } else {
        command += ".mutableAddress = nullptr,\n";
    }

    command += "}";
    return command;
}

std::string emitFieldReader(const TypeModel& owner, std::size_t typeIndex, std::size_t fieldIndex) {
    std::string command = getFieldReaderName(owner.fields[fieldIndex], typeIndex, fieldIndex);

    if (command.empty()) {
        return command;
    }

    command = "static const void* " + command + "(const engine::reflection::ObjectView& view) {\n";
    command += "    auto owner = view.tryAs<::" + owner.name + ">();\n";
    command += "    if (!owner) {\n";
    command += "        return nullptr;\n";
    command += "    }\n";
    command += "    return std::addressof(owner->" + owner.fields[fieldIndex].name + ");\n";
    command += "}\n";

    return command;
}

std::string emitFieldMutableAccessor(const TypeModel& owner, std::size_t typeIndex, std::size_t fieldIndex) {
    std::string command = getFieldMutableAccessorName(owner.fields[fieldIndex], typeIndex, fieldIndex);

    if (command.empty()) {
        return command;
    }

    command = "static void* " + command + "(const engine::reflection::ObjectView& view) {\n";
    command += "    auto owner = view.tryAsMut<::" + owner.name + ">();\n";
    command += "    if (!owner) {\n";
    command += "        return nullptr;\n";
    command += "    }\n";
    command += "    return std::addressof(owner->" + owner.fields[fieldIndex].name + ");\n";
    command += "}\n";

    return command;
}

std::string emitModifierStorage(std::span<const ModifierModel> modifiers, const std::string& prefix) {
    std::string command = "";

    for (std::size_t i = 0; i < modifiers.size(); i++) {
        command += "static constexpr auto " + prefix + "_value_" + std::to_string(i) + " = " + modifiers[i].initializationExpression + ";\n";
    }

    command += "static const std::array<engine::reflection::detail::ModifierEntry, " + std::to_string(modifiers.size()) + "> " + prefix + "_entries{";
    if (!modifiers.empty()) {
        command += "\n";
        for (std::size_t i = 0; i < modifiers.size(); i++) {
            command += "::engine::reflection::detail::ModifierEntry(" + prefix + "_value_" + std::to_string(i) + "),\n";
        }
    }
    command += "};\n";

    return command;
}

std::string emitFieldArray(const TypeModel& typeModel, std::size_t modelIndex) {
    std::string command = "";
    for (std::size_t i = 0; i < typeModel.fields.size(); i++) {
        command += emitModifierStorage(typeModel.fields[i].modifiers, "field_" + std::to_string(modelIndex) + "_" + std::to_string(i));
    }

    command += "static const std::array<engine::reflection::FieldDescriptor, " + std::to_string(typeModel.fields.size()) + "> ";
    command += "fields_" + std::to_string(modelIndex) + "{\n";
    for (std::size_t i = 0; i < typeModel.fields.size(); i++) {
        std::string readAddress = getFieldReaderName(typeModel.fields[i], modelIndex, i);
        if (!readAddress.empty()) {
            readAddress = "::engine::reflection::detail::TypeAccess<::" + typeModel.name + ">::" + readAddress;
        }
        std::string mutableAddress = getFieldMutableAccessorName(typeModel.fields[i], modelIndex, i);
        if (!mutableAddress.empty()) {
            mutableAddress = "::engine::reflection::detail::TypeAccess<::" + typeModel.name + ">::" + mutableAddress;
        }
        command += emitFieldDescriptor(typeModel.fields[i], readAddress, mutableAddress, "field_" + std::to_string(modelIndex) + "_" + std::to_string(i)) + ",\n";
    }
    command += "};";
    return command;
}

std::string emitTypeDescriptor(const TypeModel& typeModel, std::size_t modelIndex) {
    std::string prefix = "type_" + std::to_string(modelIndex);
    std::string command = emitModifierStorage(typeModel.modifiers, prefix);
    command += "const engine::reflection::TypeDescriptor ";
    command += prefix + "{\n";
    command += ".nativeTypeKey = typeid(::" + typeModel.name + "),\n";
    command += ".key = " + emitKeyExpression(typeModel.modifiers, prefix, typeModel.name) + ",\n";
    command += ".name = \"" + typeModel.displayName + "\",\n";

    command += ".modifiers = ::engine::reflection::Modifiers(" + prefix + "_entries),\n";

    command += ".category = engine::reflection::TypeCategory::Object,\n";
    command += ".fields = fields_" + std::to_string(modelIndex) + ",\n";
    command += "};\n";
    return command;
}

std::string emitTypeArray(std::size_t typeCount) {
    std::string command = "static const std::array<const engine::reflection::TypeDescriptor*, " + std::to_string(typeCount) + "> types{\n";
    for (std::size_t i = 0; i < typeCount; i++) {
        command += "&type_" + std::to_string(i) + ",\n";
    }
    command += "};\n";
    return command;
}

std::string emitSnapshotPolicyRegistration(const TypeModel& typeModel) {
    std::string command = "";
    for (auto& mod : typeModel.modifiers) {
        if (mod.qualifiedTypeName == "::engine::reflection::modifiers::Component") {
            command += "if (!snapshots.registerDefaultCopyPolicy<::" + typeModel.name + ">()) {\n";
            command += "    return false;\n";
            command += "}\n";
        }
    }
    return command;
}

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

}

std::string emitSnapshotPolicySource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName) {
    std::string command = "";
    for (auto& include : includePaths) {
        command += "#include \"" + include + "\"\n";
    }
    command += "#include <edit_snapshot_registry.h>\n\n";

    command += "namespace editor::generated::module_" + moduleName + " {\n\n";

    command += "bool registerSnapshotPolicies(::editor::EditSnapshotRegistry& snapshots) {\n";
    for (auto& type : typeModels) {
        command += emitSnapshotPolicyRegistration(type);
    }
    command += "return true;\n";
    command += "}\n\n";

    command += "}\n";
    return command;
}

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

std::string emitSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName) {
    std::string command = "";
    for (auto& include : includePaths) {
        command += "#include \"" + include + "\"\n";
    }
    command += "#include <array>\n";
    command += "#include <span>\n";
    command += "#include <memory>\n";
    command += "#include <engine/reflection/public/builtin_types.h>\n";
    command += "#include <engine/reflection/public/modifiers.h>\n";
    command += "#include <engine/reflection/public/object_view.h>\n";

    command += "\n";

    command += "namespace engine::reflection::detail {\n\n";
    for (std::size_t i = 0; i < typeModels.size(); i++) {
        command += "template<> struct TypeAccess<::" + typeModels[i].name + "> {\n";
        for (std::size_t j = 0; j < typeModels[i].fields.size(); j++) {
            command += emitFieldReader(typeModels[i], i, j);
            command += emitFieldMutableAccessor(typeModels[i], i, j);
        }
        command += "};\n";
    }
    command += "\n} // namespace engine::reflection::detail\n\n";

    command += "namespace engine::reflection::generated::module_" + moduleName + " {\n";
    command += "namespace {\n";

    for (std::size_t i = 0; i < typeModels.size(); i++) {
        command += emitFieldArray(typeModels[i], i) + "\n";
        command += emitTypeDescriptor(typeModels[i], i);
    }

    command += emitTypeArray(typeModels.size());
    command += "}\n\n";

    command += "std::span<const engine::reflection::TypeDescriptor* const> getTypes() { return types; }\n";

    command += "}\n";

    return command;
}

std::string emitReflectionBootstrapSource(const std::vector<std::string>& moduleNames) {
    std::string command = "";
    command += "#include <span>\n";
    command += "#include <engine/reflection/public/builtin_types.h>\n";
    command += "#include <engine/reflection/public/type_registry.h>\n\n";
    command += "#include <engine/reflection/public/bootstrap.h>\n\n";

    for (auto& moduleName : moduleNames) {
        command += "namespace engine::reflection::generated::module_" + moduleName + " {\n";
        command += "std::span<const engine::reflection::TypeDescriptor* const> getTypes();\n";
        command += "}\n\n";
    }

    command += "namespace engine::reflection::generated {\n\n";
    command += "bool initializeTypeRegistry(TypeRegistry& reg) {\n";
    command += "    for (auto& type : engine::reflection::getBuiltinTypes()) {\n";
    command += "        if (!reg.registerType(type)) {\n";
    command += "            return false;\n";
    command += "        }\n";
    command += "    }\n\n";
    for (auto& moduleName : moduleNames) {
        command += "    for (auto& type : engine::reflection::generated::module_" + moduleName + "::getTypes()) {\n";
        command += "        if (!reg.registerType(type)) {\n";
        command += "            return false;\n";
        command += "        }\n";
        command += "    }\n\n";
    }
    command += "    return reg.freeze();\n";
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

std::string emitSnapshotBootstrapSource(const std::vector<std::string>& moduleNames) {
    std::string command = "";
    command += "#include <snapshot_bootstrap.h>\n";

    for (auto& moduleName : moduleNames) {
        command += "namespace editor::generated::module_" + moduleName + " {\n";
        command += "bool registerSnapshotPolicies(::editor::EditSnapshotRegistry& snapshots);\n";
        command += "}\n\n";
    }

    command += "namespace editor::generated {\n\n";
    command += "bool initializeSnapshotPolicies(EditSnapshotRegistry& snapshots) {\n";
    for (auto& moduleName : moduleNames) {
        command += "    if (!editor::generated::module_" + moduleName + "::registerSnapshotPolicies(snapshots)) {\n";
        command += "        return false;\n";
        command += "    }\n\n";
    }
    command += "    return true;\n";
    command += "}\n\n";
    command += "}\n";

    return command;
}
