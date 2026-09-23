#include "model.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <analysis/markers.h>
#include <analysis/modifier_analyzer.h>
#include <analysis/type_analyzer.h>
#include <emitter.h>

#define MARKER_NAME_OBJECT "OBJECT"
#define MARKER_NAME_FIELD "FIELD"
#define MARKER_NAME_FUNCTION "FUNCTION"

class ReflectionVisitor : public clang::RecursiveASTVisitor<ReflectionVisitor> {
    DeclIndex& _declIndex;
    MarkerCheckIndex& _markerCheckIndex;

  public:
    ReflectionVisitor(DeclIndex& declIndex, MarkerCheckIndex& markerCheckIndex) : _declIndex(declIndex), _markerCheckIndex(markerCheckIndex) {}

    bool VisitDecl(clang::Decl* decl) {
        if (decl->isImplicit()) {
            return true;
        }
        auto begin = decl->getBeginLoc();
        if (begin.isInvalid() || !begin.isFileID()) {
            return true;
        }
        auto& sourceManager = decl->getASTContext().getSourceManager();
        auto fileIdAndOffset = sourceManager.getDecomposedLoc(begin);
        _declIndex[fileIdAndOffset].push_back(decl);
        return true;
    }

    bool VisitStaticAssertDecl(clang::StaticAssertDecl* decl) {
        auto location = decl->getBeginLoc();
        if (location.isInvalid()) {
            return true;
        }

        auto& sourceManager = decl->getASTContext().getSourceManager();
        auto expansionLocation = sourceManager.getExpansionLoc(location);
        if (!expansionLocation.isFileID()) {
            return true;
        }

        auto key = sourceManager.getDecomposedLoc(expansionLocation);
        auto it = _markerCheckIndex.find(key);
        if (it != _markerCheckIndex.end()) {
            it->second.push_back(decl);
        }
        return true;
    }
};

class ReflectionPPCallbacks : public clang::PPCallbacks {
    std::vector<ParsedMarker>& _markers;

  public:
    ReflectionPPCallbacks(std::vector<ParsedMarker>& markers) : _markers(markers) {}

    void MacroExpands(const clang::Token& token, const clang::MacroDefinition& d, clang::SourceRange s, const clang::MacroArgs* args) override {
        auto info = token.getIdentifierInfo();
        auto name = info->getName().str();
        if (name != MARKER_NAME_OBJECT && name != MARKER_NAME_FIELD && name != MARKER_NAME_FUNCTION) {
            return;
        }
        _markers.push_back(ParsedMarker{.name = name, .location = s});
    }
};

class ReflectionConsumer : public clang::ASTConsumer {
    const std::vector<ParsedMarker>& _markers;
    std::vector<TypeModel>& _typeModels;
    const std::vector<std::filesystem::path>& _headers;

  public:
    ReflectionConsumer(const std::vector<ParsedMarker>& markers, std::vector<TypeModel>& typeModels, const std::vector<std::filesystem::path>& headers) : _markers(markers), _typeModels(typeModels), _headers(headers) {}

    void HandleTranslationUnit(clang::ASTContext& ctx) override {
        auto& sourceManager = ctx.getSourceManager();
        auto& fileManager = sourceManager.getFileManager();
        std::set<const clang::FileEntry*> ownedFlags;

        for (auto& header : _headers) {
            auto fileRef = fileManager.getFileRef(header.generic_string());
            if (!fileRef) {
                auto& diag = ctx.getDiagnostics();
                auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "%0: %1");
                diag.Report(diagId) << header.generic_string() << llvm::toString(fileRef.takeError());
                return;
            }
            ownedFlags.emplace(&fileRef->getFileEntry());
        }

        DeclIndex declIndex{};
        MarkerCheckIndex markerCheckIndex{};

        for (auto& marker : _markers) {
            if (!marker.location.getBegin().isFileID() || !marker.location.getEnd().isFileID()) {
                auto& diag = ctx.getDiagnostics();
                auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not correct marker location");
                diag.Report(marker.location.getEnd(), diagId);
                continue;
            }
            auto key = sourceManager.getDecomposedLoc(marker.location.getBegin());

            markerCheckIndex.try_emplace(key);
        }

        ReflectionVisitor visitor{declIndex, markerCheckIndex};
        auto tud = ctx.getTranslationUnitDecl();
        visitor.TraverseDecl(tud);

        std::vector<ParsedMarker> analyzedMarkers;

        for (auto& marker : _markers) {
            auto modCheck = resolveModifierCheck(marker, markerCheckIndex, ctx);
            if (!modCheck) {
                return;
            }
            auto mods = analyzeMarkerModifiers(*modCheck, ctx);
            if (!mods) {
                return;
            }
            ParsedMarker markerWithMods = marker;
            markerWithMods.modifiers = mods.value();

            analyzedMarkers.push_back(markerWithMods);
        }

        std::map<clang::CXXRecordDecl*, ParsedMarker> objectMarkerMap{};
        std::map<clang::FieldDecl*, ParsedMarker> fieldMarkerMap{};
        std::map<clang::CXXMethodDecl*, ParsedMarker> methodMarkerMap{};

        // object run
        for (auto& marker : analyzedMarkers) {
            if (marker.name == MARKER_NAME_OBJECT) {
                auto decl = resolveMarkerTarget(marker, declIndex, ctx);
                if (!decl) {
                    continue;
                }
                auto* cxxRec = llvm::dyn_cast<clang::CXXRecordDecl>(decl);
                if (!cxxRec) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No structure or class");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!cxxRec->getIdentifier()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Name of class or structure is empty");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!cxxRec->isThisDeclarationADefinition()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Declaration is not a definition");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (cxxRec->isUnion()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Unions are not supported by this marker");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (cxxRec->isDependentContext() || cxxRec->getTemplateSpecializationKind() != clang::TSK_Undeclared) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Tamplate classes and structures are not supported by this marker");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                objectMarkerMap.emplace(cxxRec, marker);
            }
        }

        std::set<clang::CXXRecordDecl*> availableClasses{};
        for (auto& [k, v] : objectMarkerMap) {
            availableClasses.emplace(k);
        }

        // field run
        for (auto& marker : analyzedMarkers) {
            if (marker.name == MARKER_NAME_FIELD) {
                auto decl = resolveMarkerTarget(marker, declIndex, ctx);
                if (!decl) {
                    continue;
                }

                auto* fieldDecl = llvm::dyn_cast<clang::FieldDecl>(decl);
                if (!fieldDecl) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No field");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!fieldDecl->getIdentifier()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Name of field is empty");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (fieldDecl->isBitField()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Bit fields are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                auto object = llvm::dyn_cast<clang::CXXRecordDecl>(fieldDecl->getParent());
                if (!object) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "fields outside of class or structure are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!objectMarkerMap.contains(object)) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "class or structure of markered field should be markered too");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                fieldMarkerMap.emplace(fieldDecl, marker);
            }
        }

        // method run
        for (auto& marker : analyzedMarkers) {
            if (marker.name == MARKER_NAME_FUNCTION) {
                auto decl = resolveMarkerTarget(marker, declIndex, ctx);
                if (!decl) {
                    continue;
                }

                auto* methodDecl = llvm::dyn_cast<clang::CXXMethodDecl>(decl);
                if (!methodDecl) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No method");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!methodDecl->getIdentifier()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Name of method is empty");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (methodDecl->isOutOfLine()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Method is out of line");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }

                auto object = methodDecl->getParent();
                if (!object) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "method outside of class or structure are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (!objectMarkerMap.contains(object)) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "class or structure of markered method should be markered too");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (methodDecl->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Templated methods are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (methodDecl->isVariadic()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Variadic methods are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                if (methodDecl->isDeleted()) {
                    auto& diag = ctx.getDiagnostics();
                    auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Deleted methods are not supported");
                    diag.Report(marker.location.getEnd(), diagId);
                    continue;
                }
                methodMarkerMap.emplace(methodDecl, marker);
            }
        }

        for (auto [k, v] : objectMarkerMap) {
            auto fileId = sourceManager.getFileID(k->getBeginLoc());
            auto fileEntryRef = sourceManager.getFileEntryRefForID(fileId);
            if (!fileEntryRef) {
                continue;
            }
            if (!ownedFlags.contains(&fileEntryRef->getFileEntry())) {
                continue;
            }

            TypeModel typeModel{};
            typeModel.name = k->getQualifiedNameAsString();
            typeModel.displayName = k->getNameAsString();
            for (auto& modifier : v.modifiers) {
                typeModel.modifiers.push_back(modifier);
            }
            for (auto field : k->fields()) {
                if (fieldMarkerMap.contains(field)) {
                    auto fieldMarker = fieldMarkerMap.at(field);
                    FieldModel fieldModel{};
                    fieldModel.name = field->getNameAsString();
                    fieldModel.type = analyzeType(field->getType(), availableClasses);

                    switch (field->getAccess()) {
                    case clang::AS_private:
                        fieldModel.access = Access::Private;
                        break;

                    case clang::AS_protected:
                        fieldModel.access = Access::Protected;
                        break;

                    case clang::AS_public:
                        fieldModel.access = Access::Public;
                        break;

                    default:
                        break;
                    }

                    if (std::holds_alternative<std::monostate>(fieldModel.type.target)) {
                        auto& diag = ctx.getDiagnostics();
                        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "not supported field type");
                        diag.Report(field->getLocation(), diagId);
                    }

                    for (auto modifier : fieldMarker.modifiers) {
                        fieldModel.modifiers.push_back(modifier);
                    }
                    typeModel.fields.push_back(fieldModel);
                }
            }
            for (auto method : k->methods()) {
                if (methodMarkerMap.contains(method)) {
                    auto methodMarker = methodMarkerMap.at(method);
                    MethodModel methodModel{};
                    methodModel.name = method->getQualifiedNameAsString();

                    methodModel.returnType = analyzeType(method->getReturnType(), availableClasses);
                    if (std::holds_alternative<std::monostate>(methodModel.returnType.target)) {
                        auto& diag = ctx.getDiagnostics();
                        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "not supported return type");
                        diag.Report(method->getLocation(), diagId);
                        continue;
                    }

                    switch (method->getAccess()) {
                    case clang::AS_private:
                        methodModel.access = Access::Private;
                        break;

                    case clang::AS_protected:
                        methodModel.access = Access::Protected;
                        break;

                    case clang::AS_public:
                        methodModel.access = Access::Public;
                        break;

                    default:
                        break;
                    }

                    methodModel.isStatic = method->isStatic();

                    if (method->isConst()) {
                        methodModel.qualifiers |= Qualifier::Const;
                    }
                    if (method->isVolatile()) {
                        methodModel.qualifiers |= Qualifier::Volatile;
                    }

                    for (auto modifier : methodMarker.modifiers) {
                        methodModel.modifiers.push_back(modifier);
                    }

                    bool invalidParameters = false;
                    for (auto parameter : method->parameters()) {
                        ParameterModel parameterModel{};
                        parameterModel.name = parameter->getNameAsString();
                        parameterModel.type = analyzeType(parameter->getType(), availableClasses);
                        if (std::holds_alternative<std::monostate>(parameterModel.type.target)) {
                            auto& diag = ctx.getDiagnostics();
                            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "not supported parameter type");
                            diag.Report(parameter->getLocation(), diagId);
                            invalidParameters = true;
                            continue;
                        }
                        methodModel.parameters.push_back(parameterModel);
                    }
                    if (invalidParameters) {
                        continue;
                    }

                    typeModel.methods.push_back(methodModel);
                }
            }
            _typeModels.push_back(typeModel);
        }
        std::sort(_typeModels.begin(), _typeModels.end(), [](const TypeModel& a, const TypeModel& b) { return a.name < b.name; });
    }
};

class ReflectionAction : public clang::ASTFrontendAction {
    std::vector<ParsedMarker> _parsedMarkers{};
    std::vector<TypeModel>& _typeModels;
    const std::vector<std::filesystem::path>& _headers;

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, llvm::StringRef str) override {
        return std::make_unique<ReflectionConsumer>(_parsedMarkers, _typeModels, _headers);
    }

    bool BeginSourceFileAction(clang::CompilerInstance& compiler) override {
        _parsedMarkers.clear();
        compiler.getPreprocessor().addPPCallbacks(std::make_unique<ReflectionPPCallbacks>(_parsedMarkers));
        return true;
    }

  public:
    ReflectionAction(std::vector<TypeModel>& typeModels, const std::vector<std::filesystem::path>& headers) : _typeModels(typeModels), _headers(headers) {}
};

class ReflectionActionFactory : public clang::tooling::FrontendActionFactory {
    std::vector<TypeModel>& _typeModels;
    const std::vector<std::filesystem::path>& _headers;

  public:
    ReflectionActionFactory(std::vector<TypeModel>& typeModels, const std::vector<std::filesystem::path>& headers) : _typeModels(typeModels), _headers(headers) {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<ReflectionAction>(_typeModels, _headers);
    }
};

std::vector<std::filesystem::path> readHeadersManifest(const std::filesystem::path& manifest) {
    std::ifstream file(manifest);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file" + manifest.string());
    }

    std::vector<std::filesystem::path> result{};
    std::string currentPath;

    while (std::getline(file, currentPath)) {
        result.push_back(std::filesystem::path{currentPath});
    }

    if (file.bad() || !file.eof()) {
        throw std::runtime_error("Failed to read file correctly" + manifest.string());
    }

    return result;
}

std::string buildAnalysisSource(const std::vector<std::filesystem::path>& includePaths) {
    std::string command = "";
    for (auto& include : includePaths) {
        command += "#include \"" + include.generic_string() + "\"\n";
    }
    return command;
}

bool isValidModuleName(const std::string& moduleName) {
    if (moduleName.empty()) {
        return false;
    }

    if (moduleName.contains("__")) {
        return false;
    }

    if (moduleName[0] == '_') {
        return false;
    }

    for (auto c : moduleName) {
        if (c >= 'A' && c <= 'Z') {
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            continue;
        }
        if (c >= '0' && c <= '9') {
            continue;
        }
        if (c == '_') {
            continue;
        }
        return false;
    }

    return true;
}

std::vector<std::string> readModulesManifest(const std::filesystem::path& manifestPath) {
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + manifestPath.generic_string());
    }

    std::vector<std::string> result{};
    std::string line;
    int lineNum = 1;
    while (std::getline(file, line)) {
        if (!isValidModuleName(line)) {
            throw std::runtime_error("Not valid module name: " + manifestPath.generic_string() + ":" + std::to_string(lineNum));
        }
        result.push_back(line);
        lineNum++;
    }

    if (file.bad() || !file.eof()) {
        throw std::runtime_error("Failed to read file correctly" + manifestPath.string());
    }

    return result;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && argv[1] == std::string("--bootstrap")) {
        if (argc != 5 && argc != 6) {
            std::cerr << "Usage: ContinuumReflectionGenerator <--bootstrap> <manifest-file> <out-file> <out-ecs-file> [out-editor-file]" << std::endl;
            return 1;
        }

        try {
            auto modules = readModulesManifest(argv[2]);
            auto reflectionBootstrap = emitReflectionBootstrapSource(modules);
            auto reflectionEcsBootstrap = emitEcsBootstrapSource(modules);
            std::string editorbootstrap = "";
            if (argc == 6) {
                editorbootstrap = emitSnapshotBootstrapSource(modules);
            }

            std::ofstream file(argv[3]);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << argv[3];
                return 1;
            }
            file << reflectionBootstrap;
            if (file.fail()) {
                std::cerr << "failed to write into file: " << argv[3] << std::endl;
                return 1;
            }
            file.close();
            if (file.fail()) {
                std::cerr << "failed to close file: " << argv[3] << std::endl;
                return 1;
            }

            std::ofstream fileEcs(argv[4]);
            if (!fileEcs.is_open()) {
                std::cerr << "Failed to open file: " << argv[4];
                return 1;
            }
            fileEcs << reflectionEcsBootstrap;
            if (fileEcs.fail()) {
                std::cerr << "failed to write into file: " << argv[4] << std::endl;
                return 1;
            }
            fileEcs.close();
            if (fileEcs.fail()) {
                std::cerr << "failed to close file: " << argv[4] << std::endl;
                return 1;
            }

            if (argc == 6) {
                std::ofstream fileEditor(argv[5]);
                if (!fileEditor.is_open()) {
                    std::cerr << "Failed to open file: " << argv[5];
                    return 1;
                }
                fileEditor << editorbootstrap;
                if (fileEditor.fail()) {
                    std::cerr << "failed to write into file: " << argv[5] << std::endl;
                    return 1;
                }
                fileEditor.close();
                if (fileEditor.fail()) {
                    std::cerr << "failed to close file: " << argv[5] << std::endl;
                    return 1;
                }
            }
            return 0;
        } catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    }

    if (argc < 5) {
        std::cerr << "Usage: ContinuumReflectionGenerator [--manifest] <source-file> <out-file> <out-ecs-file> <module-name> [--compdb] [--] [clang-args...]" << std::endl;
        return 1;
    }

    bool manifestMode = false;
    if (argv[1] == std::string("--manifest")) {
        manifestMode = true;
    }

    int inputIndex = manifestMode ? 2 : 1;
    int outputIndex = inputIndex + 1;
    int ecsOutputIndex = inputIndex + 2;
    int moduleIndex = inputIndex + 3;
    int separatorIndex = inputIndex + 4;

    if (argc <= moduleIndex) {
        std::cerr << "Too few arguments" << std::endl;
        return 1;
    }

    std::string moduleName = argv[moduleIndex];
    if (!isValidModuleName(moduleName)) {
        std::cerr << "Not correct module name: " << moduleName << std::endl;
        return 1;
    }

    std::optional<std::filesystem::path> compilationDatabaseDir;
    if (argc > separatorIndex && std::string(argv[separatorIndex]) == "--compdb") {
        if (argc == separatorIndex + 1 || std::string(argv[separatorIndex + 1]) == "" || std::string(argv[separatorIndex + 1]) == "--") {
            std::cerr << "Not valid arguments";
            return 1;
        }
        compilationDatabaseDir = argv[separatorIndex + 1];
        separatorIndex += 2;
    }

    if (argc > separatorIndex && std::string(argv[separatorIndex]) != "--") {
        std::cerr << "No split string --" << std::endl;
        return 1;
    }

    if (compilationDatabaseDir && argc > separatorIndex + 1) {
        std::cerr << "Clang arguments are not supported in --compdb mode" << std::endl;
        return 1;
    }

    std::vector<std::string> clangArgs{
        {"-std=c++23"},
        {"-x"},
        {"c++"},
    };

    for (int i = separatorIndex + 1; i < argc; i++) {
        clangArgs.push_back(argv[i]);
    }

    try {
        std::filesystem::path inputFile = std::filesystem::absolute(argv[inputIndex]);
        std::filesystem::path outputDirectory = std::filesystem::absolute(argv[outputIndex]).parent_path();
        std::filesystem::path outputEcsDirectory = std::filesystem::absolute(argv[ecsOutputIndex]).parent_path();
        std::filesystem::path snapshotOutputPath = outputDirectory / (moduleName + ".editor_reflection.generated.cpp");

        std::vector<std::filesystem::path> headers = manifestMode ? readHeadersManifest(inputFile) : std::vector<std::filesystem::path>{inputFile};
        auto analysisSource = buildAnalysisSource(headers);

        std::filesystem::path reflectionInputPath = outputDirectory / (moduleName + ".reflection_input.cpp");

        std::unique_ptr<clang::tooling::CompilationDatabase> compilationDatabase;
        std::string foundedReflectionInputPath = "";
        if (compilationDatabaseDir) {
            auto compilationDatabaseDirAbolute = std::filesystem::absolute(compilationDatabaseDir.value());
            std::string errorMessage;

            compilationDatabase = clang::tooling::CompilationDatabase::loadFromDirectory(compilationDatabaseDirAbolute.generic_string(), errorMessage);
            if (!compilationDatabase) {
                throw std::runtime_error(compilationDatabaseDirAbolute.generic_string() + ": " + errorMessage);
            }

            auto allFilesPath = compilationDatabase->getAllFiles();
            for (auto& pathString : allFilesPath) {
                std::filesystem::path path = pathString;
                if (path.lexically_normal() == reflectionInputPath.lexically_normal()) {
                    foundedReflectionInputPath = pathString;
                }
            }
            if (foundedReflectionInputPath.empty()) {
                throw std::runtime_error("No reflection input path: " + reflectionInputPath.generic_string());
            }

            auto compileCommands = compilationDatabase->getCompileCommands(foundedReflectionInputPath);
            if (compileCommands.size() != 1) {
                throw std::runtime_error("Not available compile commands count: " + std::to_string(compileCommands.size()) + "\nfile: " + reflectionInputPath.generic_string());
            }
        }

        std::vector<TypeModel> typeModels{};
        bool success = false;

        std::vector<std::string> resourceArgs{"-resource-dir", CONTINUUM_CLANG_RESOURCE_DIR};
        if (compilationDatabase) {
            std::vector<std::string> inputFiles{foundedReflectionInputPath};
            auto tool = clang::tooling::ClangTool(*compilationDatabase, inputFiles);
            tool.mapVirtualFile(foundedReflectionInputPath, analysisSource);
            auto reflectionActionFactory = ReflectionActionFactory(typeModels, headers);
            auto adjuster = clang::tooling::getInsertArgumentAdjuster(resourceArgs, clang::tooling::ArgumentInsertPosition::END);
            tool.appendArgumentsAdjuster(adjuster);
            success = tool.run(&reflectionActionFactory) == 0;
        } else {
            clangArgs.insert(clangArgs.begin() + clangArgs.size(), resourceArgs.begin(), resourceArgs.end());
            success = clang::tooling::runToolOnCodeWithArgs(std::make_unique<ReflectionAction>(typeModels, headers), analysisSource, clangArgs, reflectionInputPath.generic_string());
        }

        if ((success)) {
            std::vector<std::string> includePaths{};
            std::vector<std::string> includeEcsPaths{};

            for (auto& header : headers) {
                includePaths.push_back(std::filesystem::relative(header, outputDirectory).generic_string());
                includeEcsPaths.push_back(std::filesystem::relative(header, outputEcsDirectory).generic_string());
            }

            auto emitedSource = emitSource(typeModels, includePaths, moduleName);
            auto emitedEcsSource = emitEcsBindingSource(typeModels, includeEcsPaths, moduleName);
            auto emitedSnapshotSource = emitSnapshotPolicySource(typeModels, includePaths, moduleName);

            std::ofstream outFile(argv[outputIndex]);
            if (!outFile.is_open()) {
                std::cerr << "failed to open file: " << argv[outputIndex] << std::endl;
                return 1;
            }
            outFile << emitedSource;
            if (outFile.fail()) {
                std::cerr << "failed to write into file: " << argv[outputIndex] << std::endl;
                return 1;
            }
            outFile.close();
            if (outFile.fail()) {
                std::cerr << "failed to close file: " << argv[outputIndex] << std::endl;
                return 1;
            }

            std::ofstream outEcsFile(argv[ecsOutputIndex]);
            if (!outEcsFile.is_open()) {
                std::cerr << "failed to open file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }
            outEcsFile << emitedEcsSource;
            if (outEcsFile.fail()) {
                std::cerr << "failed to write into file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }
            outEcsFile.close();
            if (outEcsFile.fail()) {
                std::cerr << "failed to close file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }

            std::ofstream outSnapshotPolicyFile(snapshotOutputPath);
            if (!outSnapshotPolicyFile.is_open()) {
                std::cerr << "failed to open file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
            outSnapshotPolicyFile << emitedSnapshotSource;
            if (outSnapshotPolicyFile.fail()) {
                std::cerr << "failed to write into file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
            outSnapshotPolicyFile.close();
            if (outSnapshotPolicyFile.fail()) {
                std::cerr << "failed to close file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
        }
        return success ? 0 : 1;
    } catch (const std::exception& err) {
        std::cerr << err.what();
        return 1;
    }
}