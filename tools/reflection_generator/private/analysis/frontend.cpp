#include <analysis/frontend.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Token.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

#include <analysis/markers.h>
#include <analysis/modifier_analyzer.h>
#include <analysis/type_analyzer.h>

namespace {

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

} // namespace

std::expected<std::vector<TypeModel>, std::string> analyzeHeaders(const std::vector<std::filesystem::path>& headers, std::filesystem::path reflectionInputPath, std::optional<std::filesystem::path> compilationDatabaseDir, std::vector<std::string>& clangArgs) {
    std::string analysisSource = "";
    for (auto& include : headers) {
        analysisSource += "#include \"" + include.generic_string() + "\"\n";
    }

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

    if (success) {
        return typeModels;
    } else {
        return std::unexpected("Failed to analyze sources");
    }
}
