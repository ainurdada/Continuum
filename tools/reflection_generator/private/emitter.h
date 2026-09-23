#pragma once

#include <string>
#include <vector>

#include "model.h"

std::string emitSnapshotPolicySource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitEcsBindingSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitReflectionBootstrapSource(const std::vector<std::string>& moduleNames);

std::string emitEcsBootstrapSource(const std::vector<std::string>& moduleNames);

std::string emitSnapshotBootstrapSource(const std::vector<std::string>& moduleNames);
