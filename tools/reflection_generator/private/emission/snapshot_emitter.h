#pragma once

#include <string>
#include <vector>

#include <model.h>

std::string emitSnapshotPolicySource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitSnapshotBootstrapSource(const std::vector<std::string>& moduleNames);
