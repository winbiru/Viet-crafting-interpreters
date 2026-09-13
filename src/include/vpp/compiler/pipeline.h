#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/storeString.h"
#include "vpp/compiler/codegen.h"
#include "vpp/compiler/ir.h"
#include "vpp/compiler/module_graph.h"
#include "vpp/compiler/optimizer.h"
#include "vpp/frontend/parser.h"
#include "vm/instruction.h"

namespace vietvm::compiler {

struct CompilationArtifacts {
    std::vector<vietvm::frontend::Token> tokens;
    vietvm::frontend::AstProgram ast;
    std::optional<LocalModuleSemanticIndex> moduleIndex;
    SemanticModel semantic;
    IrProgram ir;
    OptimizationReport optimization;
    std::size_t unsupportedDirectIrRegions = 0;
    std::vector<Instruction> bytecode;
};

// Owns mutable compiler registry state and the import resolution base for one
// top-level compilation. Legacy StringPool/hamMap helpers are facades over the
// context bound to the current thread, so recursive imports keep sharing this
// state without exposing it to callers.
struct CompilationContext : CompilationRegistryState {
    void clear();
};

// Runs the canonical compiler path:
// source -> lexer -> parser -> AST -> semantic analysis -> IR -> optimizer
// -> bytecode. It uses the registry state currently bound to the thread; callers that
// begin legacy/context-less top-level compilation must use resetCompilationState()
// first.
CompilationArtifacts compilePipeline(
    const std::string &source,
    const std::unordered_map<std::string, Opcode> &keywordMap,
    bool emitMainCall = true);

// Top-level entry point. It binds `context` as the active registry owner and manages
// reset/error cleanup. Recursive import compilation must keep using the context-less
// overload above so imported modules share the active session.
CompilationArtifacts compilePipeline(
    CompilationContext &context,
    const std::string &source,
    const std::unordered_map<std::string, Opcode> &keywordMap,
    bool emitMainCall = true);

} // namespace vietvm::compiler
