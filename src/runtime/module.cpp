#include "vpp/runtime/module.h"

#include <utility>

namespace vietvm::runtime {

const char *moduleStateName(ModuleState state) noexcept {
    switch (state) {
        case ModuleState::Uninitialized: return "uninitialized";
        case ModuleState::Initializing: return "initializing";
        case ModuleState::Initialized: return "initialized";
        case ModuleState::Failed: return "failed";
    }
    return "unknown";
}

bool ModuleTable::add(std::string identity,
                      std::vector<Instruction> initializer) {
    if (identity.empty() || modules_.find(identity) != modules_.end()) return false;
    RuntimeModule record{identity, std::move(initializer),
                         ModuleState::Uninitialized};
    order_.push_back(identity);
    modules_.emplace(identity, std::move(record));
    return true;
}

const RuntimeModule *ModuleTable::module(std::string_view identity) const noexcept {
    const auto found = modules_.find(std::string(identity));
    return found == modules_.end() ? nullptr : &found->second;
}

std::optional<ModuleState> ModuleTable::state(std::string_view identity) const noexcept {
    const RuntimeModule *record = module(identity);
    return record == nullptr ? std::nullopt
                             : std::optional<ModuleState>(record->state);
}

bool ModuleTable::begin(std::string_view identity) {
    auto found = modules_.find(std::string(identity));
    if (found == modules_.end() || found->second.state != ModuleState::Uninitialized) {
        return false;
    }
    found->second.state = ModuleState::Initializing;
    return true;
}

bool ModuleTable::complete(std::string_view identity) {
    auto found = modules_.find(std::string(identity));
    if (found == modules_.end() || found->second.state != ModuleState::Initializing) {
        return false;
    }
    found->second.state = ModuleState::Initialized;
    return true;
}

bool ModuleTable::fail(std::string_view identity) {
    auto found = modules_.find(std::string(identity));
    if (found == modules_.end() || found->second.state != ModuleState::Initializing) {
        return false;
    }
    found->second.state = ModuleState::Failed;
    return true;
}

} // namespace vietvm::runtime
