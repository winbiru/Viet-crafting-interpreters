#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "vm/instruction.h"

namespace vietvm::runtime {

enum class ModuleState {
    Uninitialized,
    Initializing,
    Initialized,
    Failed,
};

const char *moduleStateName(ModuleState state) noexcept;

struct RuntimeModule {
    std::string identity;
    std::vector<Instruction> initializer;
    ModuleState state = ModuleState::Uninitialized;
};

// Runtime-owned module registry. It deliberately has no compiler dependency:
// embedders may populate it from bytecode/module metadata produced elsewhere.
class ModuleTable {
public:
    bool add(std::string identity, std::vector<Instruction> initializer);
    std::optional<ModuleState> state(std::string_view identity) const noexcept;
    bool begin(std::string_view identity);
    bool complete(std::string_view identity);
    bool fail(std::string_view identity);

    const std::vector<std::string> &order() const noexcept { return order_; }
    const RuntimeModule *module(std::string_view identity) const noexcept;
    std::size_t size() const noexcept { return modules_.size(); }

private:
    std::unordered_map<std::string, RuntimeModule> modules_;
    std::vector<std::string> order_;
};

} // namespace vietvm::runtime
