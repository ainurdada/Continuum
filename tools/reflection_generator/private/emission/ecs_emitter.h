#pragma once

#include <string>
#include <vector>

#include <model.h>

std::string emitEcsBindingSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitEcsBootstrapSource(const std::vector<std::string>& moduleNames);
