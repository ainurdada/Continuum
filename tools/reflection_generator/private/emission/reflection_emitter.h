#pragma once

#include <string>
#include <vector>

#include <model.h>

std::string emitReflectionSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitReflectionBootstrapSource(const std::vector<std::string>& moduleNames);
