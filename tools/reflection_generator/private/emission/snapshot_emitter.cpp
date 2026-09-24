#include <emission/snapshot_emitter.h>

namespace {

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

} // namespace

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
