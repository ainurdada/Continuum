#pragma once

#include <optional>
#include <vector>

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>

#include <model.h>

std::optional<std::vector<ModifierModel>> analyzeMarkerModifiers(const clang::CallExpr& expr, clang::ASTContext& ctx);
