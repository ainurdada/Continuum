#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <span>

#include "model.h"

std::string getFieldReaderName(const FieldModel& field, std::size_t typeIndex, std::size_t fieldIndex);

std::string getFieldMutableAccessorName(const FieldModel& field, std::size_t typeIndex, std::size_t fieldIndex);

std::string emitFieldDescriptor(const FieldModel& model, std::string readAddress, std::string mutableAddress, const std::string& prefix);

std::string emitFieldReader(const TypeModel& owner, std::size_t typeIndex, std::size_t fieldIndex);

std::string emitFieldMutableAccessor(const TypeModel& owner, std::size_t typeIndex, std::size_t fieldIndex);

std::string emitFieldArray(const TypeModel& typeModel, std::size_t modelIndex);

std::string emitNativeTypeKey(const TypeRefModel& ref);

std::string emitTypeDescriptor(const TypeModel& typeModel, std::size_t modelIndex);

std::string emitTypeArray(std::size_t typeCount);

std::string emitSnapshotPolicyRegistration(const TypeModel& typeModel);

std::string emitComponentRegistration(const TypeModel& typeModel);

std::string emitModifierStorage(std::span<const ModifierModel> modifiers, const std::string& prefix);

std::string emitSnapshotPolicySource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitEcsBindingSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitSource(const std::vector<TypeModel>& typeModels, const std::vector<std::string>& includePaths, std::string moduleName);

std::string emitReflectionBootstrapSource(const std::vector<std::string>& moduleNames);

std::string emitEcsBootstrapSource(const std::vector<std::string>& moduleNames);

std::string emitSnapshotBootstrapSource(const std::vector<std::string>& moduleNames);
