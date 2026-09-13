#pragma once

#include <string>

#include "vpp/runtime/value.h"

namespace vietvm::helpers {

std::string escapeJsonString(const std::string &input);
bool parseJson(const std::string &input, StackValue &result, std::string &err);
bool stringifyJson(const StackValue &value, std::string &result, std::string &err);

} // namespace vietvm::helpers
