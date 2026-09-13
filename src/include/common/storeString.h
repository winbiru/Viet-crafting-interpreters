#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../vm/instruction.h"

namespace vietvm::compiler {
    struct MethodAccessInfo {
        std::string ownerClass;
        std::string visibility;
    };

    struct CompiledModuleInitializer {
        std::string identity;
        std::vector<Instruction> bytecode;
    };

    struct CompilationRegistryState {
        std::vector<std::string> stringPool;
        std::unordered_map<std::string, int> stringPoolIndexMap;
        std::unordered_map<int, std::vector<Instruction>> functionBytecode;
        std::unordered_map<int, int> functionNameIndices;
        std::unordered_set<std::string> importedFiles;
        std::vector<CompiledModuleInitializer> moduleInitializers;
        std::unordered_map<std::string, MethodAccessInfo> methodAccess;
        std::vector<std::string> classContextStack;
        // Base directory used to resolve relative imports for this compilation.
        // This is configuration, so clear() intentionally preserves it.
        std::filesystem::path importResolutionBase;
        int nextFunctionId = 0;

        void clear() {
            stringPool.clear();
            stringPoolIndexMap.clear();
            functionBytecode.clear();
            functionNameIndices.clear();
            importedFiles.clear();
            moduleInitializers.clear();
            methodAccess.clear();
            classContextStack.clear();
            nextFunctionId = 0;
        }
    };

    CompilationRegistryState &activeCompilationRegistryState();
    CompilationRegistryState *setActiveCompilationRegistryState(
        CompilationRegistryState *state);

    class hamMap {
    public:
        static std::unordered_map<int, std::vector<Instruction>> &bytecodeMap();
        static std::unordered_map<int, int> &nameIndexMap();
        static void setHamNameIndex(int hamId, int nameIndex) { nameIndexMap()[hamId] = nameIndex; }
        static void clearHamNameIndexMap() { nameIndexMap().clear(); }
        static int allocHamId();
        static void resetHamIdCounter();
    };
    class StringPool {
    public:
        // returns existing index if string exists (dedupe), otherwise pushes new string
        static int storeString(const std::string& s);
        // returns index if exists, -1 if not found (does not create)
        static int findString(const std::string& s);
        static const std::string& getString(int idx);
        static size_t size() noexcept;
        static const std::vector<std::string>& getPool();
        static void clear();
    };

} // namespace vietvm::compiler
