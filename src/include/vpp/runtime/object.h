#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "vpp/runtime/value.h"

namespace vietvm::runtime {

ClassHandle createClass(std::string name, ClassHandle superclass = nullptr);
bool defineMethod(const ClassHandle &klass, std::string name, int functionId);
std::optional<int> lookupMethod(const ClassHandle &klass, std::string_view name);
bool isSubclassOf(const ClassHandle &klass, const ClassHandle &candidateBase);

InstanceHandle createInstance(ClassHandle klass);
bool setInstanceField(const InstanceHandle &instance,
                      std::string name,
                      StackValue value);
std::optional<StackValue> getInstanceField(const InstanceHandle &instance,
                                           std::string_view name);
bool hasInstanceField(const InstanceHandle &instance, std::string_view name);

} // namespace vietvm::runtime
