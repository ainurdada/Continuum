#pragma once

#include <set>

#include <clang/AST/Type.h>

#include <model.h>

TypeRefModel analyzeType(const clang::QualType& type, const std::set<clang::CXXRecordDecl*>& availableClasses);