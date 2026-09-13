#pragma once

#include <string>
#include <vector>

#include "vpp/runtime/value.h"

namespace vietvm::helpers {

// Native primitives shared by the core, I/O, and system standard-library
// facades. Keeping these below VM lets both CLI and embedded runtimes expose
// the same behavior without console dependencies.
bool handleNativeFoundationFunction(const std::string &fn,
                                    const std::vector<StackValue> &args,
                                    StackValue &result,
                                    std::string &err);

} // namespace vietvm::helpers
