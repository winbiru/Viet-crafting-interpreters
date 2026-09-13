#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "vpp/compiler/semantic.h"
#include "vpp/frontend/ast.h"

namespace vietvm::compiler {

// A resolved local import has a lexical identity on purpose.  Symlink-aware
// canonicalization would change the identity historically used by the import
// compiler and can also fail for a missing target before a useful diagnostic
// is produced.
struct LocalModuleLocation {
    std::filesystem::path path;
    std::string identity;
};

struct LocalModuleSource {
    std::filesystem::path path;
    std::string identity;
    std::string source;
};

// Resolves only the first structured import cohort: explicit local .vi files.
// Callers obtain AstImportSpec from an AstImportForm::LocalSourceFile
// statement; package and bare-module lookup remain outside this API for now.
class LocalModuleResolver {
public:
    explicit LocalModuleResolver(
        std::filesystem::path resolutionBase = std::filesystem::current_path());

    const std::filesystem::path &resolutionBase() const noexcept;
    LocalModuleLocation resolve(
        const vietvm::frontend::AstImportSpec &importSpec) const;
    LocalModuleSource read(const LocalModuleLocation &location) const;

private:
    std::filesystem::path resolutionBase_;
};

enum class LocalModuleEdgeAction {
    Load,
    DuplicateNoOp,
    CycleNoOp,
};

const char *localModuleEdgeActionName(LocalModuleEdgeAction action) noexcept;

// Edges include no-op encounters as well as first loads.  This makes source
// order, duplicate suppression, cycles, and alias metadata independently
// observable without consulting compiler-global state.
struct LocalModuleEdge {
    std::string importerIdentity;
    std::string importedIdentity;
    vietvm::frontend::AstImportSpec importSpec;
    LocalModuleEdgeAction action = LocalModuleEdgeAction::Load;
};

struct LocalModuleGraph {
    std::string entryIdentity;
    // Unique first loads in depth-first preorder.
    std::vector<LocalModuleSource> modules;
    // Every import encounter in the same traversal order, including no-ops.
    std::vector<LocalModuleEdge> edges;
};

using LocalModuleImportScanner = std::function<
    std::vector<vietvm::frontend::AstImportSpec>(
        const std::string &source,
        const std::filesystem::path &sourcePath)>;

// Builds an ordered graph from already-structured import metadata.  Parsing
// is injected so this foundation stays independent of the compiler pipeline;
// a later integration can scan each loaded module's AstProgram here.
class LocalModuleGraphBuilder {
public:
    LocalModuleGraphBuilder(LocalModuleResolver resolver,
                            LocalModuleImportScanner importScanner);

    LocalModuleGraph build(
        std::string entryIdentity,
        const std::vector<vietvm::frontend::AstImportSpec> &rootImports) const;

private:
    LocalModuleResolver resolver_;
    LocalModuleImportScanner importScanner_;
};

// Public surface discovered from one source module. Until V++ gains an
// explicit `export` statement, top-level functions/classes keep the historical
// behavior: unspecified/public declarations are exported, private/protected
// declarations are module-internal.
struct ModuleExportSymbol {
    std::string name;
    SemanticSymbolKind kind = SemanticSymbolKind::Function;
    vietvm::frontend::SourceSpan declaration{};
};

struct LocalModuleSemanticRecord {
    std::filesystem::path path;
    std::string identity;
    std::vector<ModuleExportSymbol> exports;
};

// Compile-time module index. The graph owns import identity/order while this
// layer adds the namespace surface consumed by semantic analysis.
struct LocalModuleSemanticIndex {
    std::string entryIdentity;
    LocalModuleGraph graph;
    std::vector<LocalModuleSemanticRecord> modules;

    const LocalModuleSemanticRecord *module(std::string_view identity) const noexcept;
    const ModuleExportSymbol *exportedSymbol(std::string_view moduleIdentity,
                                             std::string_view name) const noexcept;

    // Produces only direct imports of `importerIdentity`. An import alias
    // qualifies the exported name (`alias.symbol`); an unaliased import keeps
    // the legacy flat namespace behavior.
    SemanticEnvironment semanticEnvironmentFor(
        std::string_view importerIdentity) const;
};

enum class ModuleIndexMode {
    DirectOnly,
    Recursive,
};

// Builds import graph + export surface from the structured AST. Package/bare
// module resolution still belongs to the package resolver; this phase covers
// explicit local .vi imports only.
LocalModuleSemanticIndex buildLocalModuleSemanticIndex(
    LocalModuleResolver resolver,
    std::string entryIdentity,
    const std::vector<vietvm::frontend::AstImportSpec> &rootImports,
    ModuleIndexMode mode = ModuleIndexMode::Recursive);

enum class ModuleInitializationState {
    Uninitialized,
    Initializing,
    Initialized,
    Failed,
};

const char *moduleInitializationStateName(
    ModuleInitializationState state) noexcept;

// Runtime-facing lifecycle contract. It is intentionally independent of VM
// storage so object-model/GC work can later make the module table a GC root
// without changing the transition semantics.
class ModuleInitializationTracker {
public:
    explicit ModuleInitializationTracker(const LocalModuleSemanticIndex &index);

    std::optional<ModuleInitializationState> state(
        std::string_view identity) const noexcept;
    bool begin(std::string_view identity);
    bool complete(std::string_view identity);
    bool fail(std::string_view identity);

private:
    std::unordered_map<std::string, ModuleInitializationState> states_;
};

} // namespace vietvm::compiler
