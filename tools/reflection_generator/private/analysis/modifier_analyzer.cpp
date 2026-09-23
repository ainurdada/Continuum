#include <analysis/modifier_analyzer.h>

#include <clang/AST/ExprCXX.h>
#include <clang/AST/QualTypeNames.h>
#include <clang/Basic/SourceManager.h>

namespace {

const clang::DeclRefExpr* getModifierCheckReference(const clang::CallExpr& call) {
    auto callee = call.getCallee();
    if (!callee) {
        return nullptr;
    }

    auto ref = llvm::dyn_cast<clang::DeclRefExpr>(callee->IgnoreParenImpCasts());
    if (!ref || !ref->hasExplicitTemplateArgs()) {
        return nullptr;
    }

    return ref;
}

const clang::CXXNewExpr* getModifierConstruction(const clang::TemplateArgumentLoc& temp) {
    auto info = temp.getTypeSourceInfo();
    if (!info) {
        return nullptr;
    }
    auto ser = info->getTypeLoc().getAs<clang::TemplateSpecializationTypeLoc>();
    if (!ser || ser.getNumArgs() != 1) {
        return nullptr;
    }

    auto typeInfo = ser.getArgLoc(0).getTypeSourceInfo();
    if (!typeInfo) {
        return nullptr;
    }

    auto declt = typeInfo->getTypeLoc().getAs<clang::DecltypeTypeLoc>();
    if (!declt) {
        return nullptr;
    }

    auto under = declt.getUnderlyingExpr();
    if (!under) {
        return nullptr;
    }

    return llvm::dyn_cast<clang::CXXNewExpr>(under->IgnoreParenImpCasts());
}

std::string getModifierTypeName(clang::QualType type, const clang::ASTContext& ctx) {
    return clang::TypeName::getFullyQualifiedName(type.getCanonicalType().getNonReferenceType().getUnqualifiedType(), ctx, ctx.getPrintingPolicy(), true);
}

std::optional<std::string> analyzeModifierArgument(const clang::Expr& expr, clang::ASTContext& ctx) {
    auto clearExpr = expr.IgnoreUnlessSpelledInSource();
    auto boo = llvm::dyn_cast<clang::CXXBoolLiteralExpr>(clearExpr);
    if (boo) {
        return boo->getValue() ? "true" : "false";
    }
    auto uo = llvm::dyn_cast<clang::UnaryOperator>(clearExpr);
    if (uo) {
        switch (uo->getOpcode()) {
        case clang::UO_Plus:
        case clang::UO_Minus:
            break;

        default:
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        if (!uo->getType()->isIntegerType() && !uo->getType()->isRealFloatingType()) {
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        auto res = analyzeModifierArgument(*uo->getSubExpr(), ctx);
        if (!res) {
            return std::nullopt;
        }
        switch (uo->getOpcode()) {
        case clang::UO_Plus:
            return "+(" + res.value() + ")";
        case clang::UO_Minus:
            return "-(" + res.value() + ")";
        default:
            break;
        }
    }
    auto flo = llvm::dyn_cast<clang::FloatingLiteral>(clearExpr);
    if (flo) {
        if (!flo->getValue().isFinite()) {
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        std::string res;
        llvm::raw_string_ostream stream(res);
        flo->printPretty(stream, nullptr, ctx.getPrintingPolicy());
        return res;
    }
    auto integer = llvm::dyn_cast<clang::IntegerLiteral>(clearExpr);
    if (integer) {
        auto builtin = integer->getType()->getAs<clang::BuiltinType>();
        if (!builtin) {
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        switch (builtin->getKind()) {
        case clang::BuiltinType::Int:
        case clang::BuiltinType::UInt:
        case clang::BuiltinType::Long:
        case clang::BuiltinType::ULong:
        case clang::BuiltinType::LongLong:
        case clang::BuiltinType::ULongLong:
            break;

        default:
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        std::string res;
        llvm::raw_string_ostream stream(res);
        integer->printPretty(stream, nullptr, ctx.getPrintingPolicy());
        return res;
    }
    auto str = llvm::dyn_cast<clang::StringLiteral>(clearExpr);
    if (!str || !str->isOrdinary()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier argument");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    std::string res;
    llvm::raw_string_ostream stream(res);
    str->outputString(stream);

    return res;
}

std::optional<ModifierModel> analyzeModifier(const clang::CXXNewExpr& expr, clang::ASTContext& ctx) {
    auto type = expr.getAllocatedType();
    auto record = type->getAsCXXRecordDecl();
    if (!record) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier type");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    if (!record->getIdentifier()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not modifier name found");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }
    if (record->isLocalClass()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid modifier");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }
    if (record->isInAnonymousNamespace()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Modifier is on anonymous namespace");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    if (expr.isArray() || expr.getNumPlacementArgs() != 0) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "expr is array");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    std::string argumentsText = "";

    switch (expr.getInitializationStyle()) {
    case clang::CXXNewInitializationStyle::None:
        break;
    case clang::CXXNewInitializationStyle::Parens: {
        auto con = expr.getConstructExpr();
        if (!con) {
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid construct");
            diag.Report(expr.getBeginLoc(), diagId);
            return std::nullopt;
        }
        for (auto arg : con->arguments()) {
            if (arg->isDefaultArgument()) {
                continue;
            }
            auto analyzedArgument = analyzeModifierArgument(*arg, ctx);
            if (!analyzedArgument) {
                return std::nullopt;
            }
            if (!argumentsText.empty()) {
                argumentsText += ", ";
            }
            argumentsText += analyzedArgument.value();
        }
    } break;
    case clang::CXXNewInitializationStyle::Braces:
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not valid construct");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    std::string modifierName = getModifierTypeName(type, ctx);
    return ModifierModel{.qualifiedTypeName = modifierName, .initializationExpression = modifierName + "(" + argumentsText + ")"};
}

} // namespace

std::optional<std::vector<ModifierModel>> analyzeMarkerModifiers(const clang::CallExpr& expr, clang::ASTContext& ctx) {
    auto& sourceManager = ctx.getSourceManager();

    auto ref = getModifierCheckReference(expr);
    if (!ref) {
        auto& diag = sourceManager.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not correct form");
        diag.Report(expr.getBeginLoc(), diagId);
        return std::nullopt;
    }

    std::vector<ModifierModel> result{};
    for (auto argument : ref->template_arguments()) {
        auto con = getModifierConstruction(argument);
        if (!con) {
            auto& diag = ctx.getDiagnostics();
            auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not correct form");
            diag.Report(argument.getLocation(), diagId);
            return std::nullopt;
        }
        auto analyze = analyzeModifier(*con, ctx);
        if (!analyze) {
            return std::nullopt;
        }
        result.push_back(analyze.value());
    }

    return result;
}