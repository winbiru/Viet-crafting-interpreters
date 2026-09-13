#include "common/storeString.h"
#include "vpp/core/message_constants.h"
#include <stdexcept>
#include <vector>
#include <iostream>

namespace vietvm::compiler {

    namespace {
        thread_local CompilationRegistryState defaultRegistryState;
        thread_local CompilationRegistryState *activeRegistryState = &defaultRegistryState;
    }

    CompilationRegistryState &activeCompilationRegistryState() {
        return *activeRegistryState;
    }

    CompilationRegistryState *setActiveCompilationRegistryState(
        CompilationRegistryState *state) {
        CompilationRegistryState *previous = activeRegistryState;
        activeRegistryState = state == nullptr ? &defaultRegistryState : state;
        return previous;
    }

    std::unordered_map<int, std::vector<Instruction>> &hamMap::bytecodeMap() {
        return activeCompilationRegistryState().functionBytecode;
    }

    std::unordered_map<int, int> &hamMap::nameIndexMap() {
        return activeCompilationRegistryState().functionNameIndices;
    }

    int hamMap::allocHamId() {
        return activeCompilationRegistryState().nextFunctionId++;
    }

    void hamMap::resetHamIdCounter() {
        activeCompilationRegistryState().nextFunctionId = 0;
    }

    int StringPool::findString(const std::string& s) {
        auto &state = activeCompilationRegistryState();
        auto it = state.stringPoolIndexMap.find(s);
        if (it != state.stringPoolIndexMap.end()) return it->second;
        return -1;
    }

    int StringPool::storeString(const std::string& s) {
        auto &state = activeCompilationRegistryState();
        // fast path: return existing index if present
        auto it = state.stringPoolIndexMap.find(s);
        if (it != state.stringPoolIndexMap.end()) {
            return it->second;
        }
        // else add
        state.stringPool.push_back(s);
        int idx = static_cast<int>(state.stringPool.size() - 1);
        state.stringPoolIndexMap.emplace(s, idx);
        // debug log (temporary) to show additions
        // std::cerr << "DEBUG: StringPool added [" << idx << "] = \"" << s << "\"\n";
        return idx;
    }

    const std::string& StringPool::getString(int idx)  {
        auto &pool = activeCompilationRegistryState().stringPool;
        if (idx < 0 || static_cast<size_t>(idx) >= pool.size()) {
            throw std::out_of_range(vietvm::messages::formatMessage(
                vietvm::messages::kInternalStringPoolIndex));
        }
        return pool[idx];
    }
    const std::vector<std::string>& StringPool::getPool() {
        return activeCompilationRegistryState().stringPool;
    }
    size_t StringPool::size() noexcept {
        return activeCompilationRegistryState().stringPool.size();
    }
    void StringPool::clear() {
        auto &state = activeCompilationRegistryState();
        state.stringPool.clear();
        state.stringPoolIndexMap.clear();
    }
} // namespace vietvm::compiler
