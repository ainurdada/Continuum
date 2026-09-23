#include <analysis/type_analyzer.h>

#include <unordered_map>

#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclTemplate.h>

namespace {

// clang-format off
const std::unordered_map<clang::BuiltinType::Kind, BuiltinKind> clangBuiltinTypeConverts{
    {clang::BuiltinType::Void,BuiltinKind::Void},
    {clang::BuiltinType::Bool, BuiltinKind::Bool},
    {clang::BuiltinType::Int, BuiltinKind::Int},
    {clang::BuiltinType::Float, BuiltinKind::Float},
    {clang::BuiltinType::Double, BuiltinKind::Double},
};
// clang-format on

bool isCharTemplateArgument(const clang::TemplateArgument& arg, const clang::ASTContext& ctx) {
    if (arg.getKind() != clang::TemplateArgument::Type) {
        return false;
    }
    auto type = arg.getAsType();
    return ctx.hasSameType(type, ctx.CharTy);
}

const clang::ClassTemplateSpecializationDecl* findStdTemplateSpecialization(const clang::CXXRecordDecl& decl, llvm::StringRef str) {
    auto classSpec = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(&decl);
    if (!classSpec) {
        return nullptr;
    }
    auto specTemp = classSpec->getSpecializedTemplate();
    if (!specTemp || specTemp->getName() != str || !specTemp->isInStdNamespace()) {
        return nullptr;
    }
    return classSpec;
}

bool isStdCharSpecialization(const clang::CXXRecordDecl& decl, llvm::StringRef str) {
    auto spec = findStdTemplateSpecialization(decl, str);
    if (!spec) {
        return false;
    }
    const auto& args = spec->getTemplateArgs();
    if (args.size() != 1) {
        return false;
    }
    return isCharTemplateArgument(args[0], decl.getASTContext());
}

bool isStdString(const clang::CXXRecordDecl& decl) {
    auto spec = findStdTemplateSpecialization(decl, "basic_string");
    if (!spec) {
        return false;
    }
    const auto& args = spec->getTemplateArgs();
    if (args.size() != 3 || !isCharTemplateArgument(args[0], decl.getASTContext()) || args[1].getKind() != clang::TemplateArgument::Type || args[2].getKind() != clang::TemplateArgument::Type) {
        return false;
    }
    auto type1 = args[1].getAsType();
    if (type1.hasQualifiers()) {
        return false;
    }
    auto t1decl = type1->getAsCXXRecordDecl();
    if (!t1decl || !isStdCharSpecialization(*t1decl, "char_traits")) {
        return false;
    }
    auto type2 = args[2].getAsType();
    if (type2.hasQualifiers()) {
        return false;
    }
    auto t2decl = type2->getAsCXXRecordDecl();
    if (!t2decl || !isStdCharSpecialization(*t2decl, "allocator")) {
        return false;
    }
    return true;
}

}

TypeRefModel analyzeType(const clang::QualType& type, const std::set<clang::CXXRecordDecl*>& availableClasses) {
    TypeRefModel result{};
    result.spelling = type.getAsString();

    const clang::QualType canonicalType = type.getCanonicalType();

    if (const auto* record = canonicalType->getAsCXXRecordDecl()) {
        if (isStdString(*record)) {
            result.target = BuiltinKind::String;
        } else {
            auto recordDef = record->getDefinition();
            if (recordDef && availableClasses.contains(recordDef)) {
                RecordTypeRefModel recTypeRef{};
                recTypeRef.qualifiedName = recordDef->getQualifiedNameAsString();
                result.target = recTypeRef;
            }
        }
    } else if (const auto* builtin = canonicalType->getAs<clang::BuiltinType>()) {
        if (clangBuiltinTypeConverts.contains(builtin->getKind())) {
            result.target = clangBuiltinTypeConverts.at(builtin->getKind());
        }
    }

    if (canonicalType.isConstQualified()) {
        result.qualifiers |= Qualifier::Const;
    }

    if (canonicalType.isVolatileQualified()) {
        result.qualifiers |= Qualifier::Volatile;
    }

    return result;
}
