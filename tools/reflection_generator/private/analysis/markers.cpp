#include <analysis/markers.h>

#include <clang/AST/ExprCXX.h>
#include <clang/Lex/Lexer.h>

namespace {

const clang::LambdaExpr* getMarkerLambda(const clang::StaticAssertDecl& decl) {
    auto assertExpr = decl.getAssertExpr();
    if (!assertExpr) {
        return nullptr;
    }
    auto op = llvm::dyn_cast<clang::CXXOperatorCallExpr>(assertExpr->IgnoreParenImpCasts());
    if (!op) {
        return nullptr;
    }
    if (op->getOperator() != clang::OO_Call) {
        return nullptr;
    }
    if (op->getNumArgs() != 1) {
        return nullptr;
    }

    return llvm::dyn_cast<clang::LambdaExpr>(op->getArg(0)->IgnoreImplicit());
}

const clang::CallExpr* getModifierCheckCall(const clang::StaticAssertDecl& decl) {
    auto expr = getMarkerLambda(decl);
    if (!expr) {
        return nullptr;
    }

    auto body = expr->getBody();
    auto comp = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!comp) {
        return nullptr;
    }

    auto back = comp->body_back();
    if (!back) {
        return nullptr;
    }

    auto ret = llvm::dyn_cast<clang::ReturnStmt>(back);
    if (!ret) {
        return nullptr;
    }

    auto retVal = ret->getRetValue();
    if (!retVal) {
        return nullptr;
    }

    auto callExpr = llvm::dyn_cast<clang::CallExpr>(retVal->IgnoreParenImpCasts());
    if (!callExpr) {
        return nullptr;
    }

    auto funcDecl = callExpr->getDirectCallee();
    if (!funcDecl) {
        return nullptr;
    }

    if (funcDecl->getQualifiedNameAsString() != "engine::reflection::modifiers::validateModifiers") {
        return nullptr;
    }

    return callExpr;
}

} // namespace

clang::Decl* resolveMarkerTarget(const ParsedMarker& marker, const DeclIndex& declIndex, clang::ASTContext& ctx) {
    auto& sourceManager = ctx.getSourceManager();
    if (!marker.location.getBegin().isFileID() || !marker.location.getEnd().isFileID()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not correct marker location");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    auto nextToken = clang::Lexer::findNextToken(marker.location.getEnd(), sourceManager, ctx.getLangOpts(), false);
    if (!nextToken || nextToken.value().getKind() == clang::tok::eof) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No marker target");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    auto nextTokenKey = sourceManager.getDecomposedLoc(nextToken->getLocation());

    auto it = declIndex.find(nextTokenKey);
    if (it == declIndex.end()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No marker target");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    if (it->second.size() != 1) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not single marker target");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    return it->second.front();
}

const clang::CallExpr* resolveModifierCheck(const ParsedMarker& marker, const MarkerCheckIndex& markerCheckIndex, clang::ASTContext& ctx) {
    auto& sourceManager = ctx.getSourceManager();
    if (!marker.location.getBegin().isFileID() || !marker.location.getEnd().isFileID()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not correct mofifier location");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    auto markerKey = sourceManager.getDecomposedLoc(marker.location.getBegin());

    auto it = markerCheckIndex.find(markerKey);
    if (it == markerCheckIndex.end()) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "No mofifier target");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    if (it->second.size() != 1) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Not unique mofifier");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }

    auto res = getModifierCheckCall(*it->second[0]);
    if (!res) {
        auto& diag = ctx.getDiagnostics();
        auto diagId = diag.getCustomDiagID(clang::DiagnosticsEngine::Error, "Marker check doesn't have validateModifiers");
        diag.Report(marker.location.getEnd(), diagId);
        return nullptr;
    }
    return res;
}
