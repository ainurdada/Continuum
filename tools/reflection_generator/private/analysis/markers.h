#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/Basic/SourceManager.h>

#include <model.h>

using DeclIndex = std::map<std::pair<clang::FileID, unsigned>, std::vector<clang::Decl*>>;
using MarkerCheckIndex = std::map<std::pair<clang::FileID, unsigned>, std::vector<clang::StaticAssertDecl*>>;

struct ParsedMarker {
    std::string name;
    std::vector<ModifierModel> modifiers;
    clang::SourceRange location;
};

clang::Decl* resolveMarkerTarget(const ParsedMarker& marker, const DeclIndex& declIndex, clang::ASTContext& ctx);

const clang::CallExpr* resolveModifierCheck(const ParsedMarker& marker, const MarkerCheckIndex& markerCheckIndex, clang::ASTContext& ctx);
