#include "vpp/compiler/module_graph.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include "frontend/lexer.h"
#include "vpp/core/message_constants.h"
#include "vpp/core/project_layout.h"
#include "vpp/frontend/parser.h"

namespace vietvm::compiler {
namespace {

namespace fs = std::filesystem;

fs::path absoluteLexical(const fs::path &path) {
    try {
        return fs::absolute(path).lexically_normal();
    } catch (...) {
        return path.lexically_normal();
    }
}

enum class VisitState {
    Active,
    Loaded,
};

vietvm::frontend::AstProgram parseModuleSource(const std::string &source) {
    return vietvm::frontend::parseTokens(
        postProcessTokensWithSpans(tokenizeWithSpans(source)));
}

std::vector<vietvm::frontend::AstImportSpec> structuredLocalImports(
    const vietvm::frontend::AstProgram &program) {
    std::vector<vietvm::frontend::AstImportSpec> imports;
    for (const vietvm::frontend::AstStatement &statement : program.statements) {
        if (statement.kind != vietvm::frontend::AstStatementKind::Import ||
            statement.importForm !=
                vietvm::frontend::AstImportForm::LocalSourceFile ||
            statement.importSpec.target.empty() ||
            !statement.importSpec.hasSemicolon) {
            continue;
        }
        const fs::path target = vietvm::core::utf8Path(statement.importSpec.target);
        if (target.extension() != ".vi") {
            // Bare/package imports are resolved by the package resolver. The
            // local module graph deliberately indexes explicit source files.
            continue;
        }
        imports.push_back(statement.importSpec);
    }
    return imports;
}

bool isModuleExportVisibility(vietvm::frontend::AstVisibility visibility) noexcept {
    return visibility == vietvm::frontend::AstVisibility::Unspecified ||
           visibility == vietvm::frontend::AstVisibility::Public;
}

std::vector<ModuleExportSymbol> moduleExports(
    const vietvm::frontend::AstProgram &program) {
    std::vector<ModuleExportSymbol> exports;
    for (const vietvm::frontend::AstStatement &statement : program.statements) {
        if (statement.declarationName.empty() ||
            !isModuleExportVisibility(statement.visibility)) {
            continue;
        }

        SemanticSymbolKind kind;
        if (statement.kind == vietvm::frontend::AstStatementKind::Function) {
            kind = SemanticSymbolKind::Function;
        } else if (statement.kind == vietvm::frontend::AstStatementKind::Class) {
            kind = SemanticSymbolKind::Class;
        } else {
            continue;
        }
        exports.push_back(
            ModuleExportSymbol{statement.declarationName, kind, statement.span});
    }
    return exports;
}

struct BuildContext {
    const LocalModuleResolver &resolver;
    const LocalModuleImportScanner &scanner;
    LocalModuleGraph graph;
    std::unordered_map<std::string, VisitState> states;

    void visit(const std::string &importerIdentity,
               const vietvm::frontend::AstImportSpec &importSpec) {
        const LocalModuleLocation location = resolver.resolve(importSpec);

        LocalModuleEdge edge;
        edge.importerIdentity = importerIdentity;
        edge.importedIdentity = location.identity;
        edge.importSpec = importSpec;

        const auto existing = states.find(location.identity);
        if (existing != states.end()) {
            edge.action = existing->second == VisitState::Active
                              ? LocalModuleEdgeAction::CycleNoOp
                              : LocalModuleEdgeAction::DuplicateNoOp;
            graph.edges.push_back(std::move(edge));
            return;
        }

        // Mark before reading or parsing.  A recursive import can now observe
        // this identity as Active, while any failure below rolls the mark back.
        states.emplace(location.identity, VisitState::Active);
        edge.action = LocalModuleEdgeAction::Load;
        graph.edges.push_back(std::move(edge));

        try {
            LocalModuleSource module = resolver.read(location);
            const std::vector<vietvm::frontend::AstImportSpec> imports =
                scanner(module.source, module.path);
            graph.modules.push_back(std::move(module));

            for (const vietvm::frontend::AstImportSpec &nestedImport : imports) {
                visit(location.identity, nestedImport);
            }
            states[location.identity] = VisitState::Loaded;
        } catch (...) {
            states.erase(location.identity);
            throw;
        }
    }
};

} // namespace

LocalModuleResolver::LocalModuleResolver(fs::path resolutionBase)
    : resolutionBase_(absoluteLexical(std::move(resolutionBase))) {}

const fs::path &LocalModuleResolver::resolutionBase() const noexcept {
    return resolutionBase_;
}

LocalModuleLocation LocalModuleResolver::resolve(
    const vietvm::frontend::AstImportSpec &importSpec) const {
    const fs::path requested = vietvm::core::utf8Path(importSpec.target);
    // `resolutionBase_` defaults to the process cwd for legacy compatibility,
    // but an explicit base is authoritative (and makes graph construction
    // deterministic for embedders and tests whose process cwd is elsewhere).
    fs::path resolved = absoluteLexical(resolutionBase_ / requested);

    if (!fs::exists(resolved)) {
        for (fs::path directory = resolutionBase_;;
             directory = directory.parent_path()) {
            const fs::path candidate = directory / requested;
            if (fs::exists(candidate)) {
                resolved = absoluteLexical(candidate);
                break;
            }
            if (directory == directory.parent_path()) break;
        }
    }

    resolved = resolved.lexically_normal();
    return {resolved, resolved.u8string()};
}

LocalModuleSource LocalModuleResolver::read(
    const LocalModuleLocation &location) const {
    std::ifstream input(location.path);
    if (!input.is_open()) {
        throw std::runtime_error(vietvm::messages::formatMessage(
            vietvm::messages::kImportCannotOpenFile, {location.identity}));
    }

    std::ostringstream source;
    source << input.rdbuf();
    if (input.bad()) {
        throw std::runtime_error(vietvm::messages::formatMessage(
            vietvm::messages::kImportCannotOpenFile, {location.identity}));
    }
    return {location.path, location.identity, source.str()};
}

const char *localModuleEdgeActionName(LocalModuleEdgeAction action) noexcept {
    switch (action) {
        case LocalModuleEdgeAction::Load: return "load";
        case LocalModuleEdgeAction::DuplicateNoOp: return "duplicate_no_op";
        case LocalModuleEdgeAction::CycleNoOp: return "cycle_no_op";
    }
    return "unknown";
}

LocalModuleGraphBuilder::LocalModuleGraphBuilder(
    LocalModuleResolver resolver,
    LocalModuleImportScanner importScanner)
    : resolver_(std::move(resolver)),
      importScanner_(std::move(importScanner)) {
    if (!importScanner_) {
        throw std::invalid_argument(
            "LocalModuleGraphBuilder requires an import scanner");
    }
}

LocalModuleGraph LocalModuleGraphBuilder::build(
    std::string entryIdentity,
    const std::vector<vietvm::frontend::AstImportSpec> &rootImports) const {
    BuildContext context{resolver_, importScanner_, {}, {}};
    context.graph.entryIdentity = std::move(entryIdentity);
    for (const vietvm::frontend::AstImportSpec &rootImport : rootImports) {
        context.visit(context.graph.entryIdentity, rootImport);
    }
    return std::move(context.graph);
}

const LocalModuleSemanticRecord *LocalModuleSemanticIndex::module(
    std::string_view identity) const noexcept {
    for (const LocalModuleSemanticRecord &candidate : modules) {
        if (candidate.identity == identity) return &candidate;
    }
    return nullptr;
}

const ModuleExportSymbol *LocalModuleSemanticIndex::exportedSymbol(
    std::string_view moduleIdentity,
    std::string_view name) const noexcept {
    const LocalModuleSemanticRecord *record = module(moduleIdentity);
    if (record == nullptr) return nullptr;
    for (const ModuleExportSymbol &symbol : record->exports) {
        if (symbol.name == name) return &symbol;
    }
    return nullptr;
}

SemanticEnvironment LocalModuleSemanticIndex::semanticEnvironmentFor(
    std::string_view importerIdentity) const {
    SemanticEnvironment environment;
    std::unordered_set<std::string> seen;

    for (const LocalModuleEdge &edge : graph.edges) {
        if (edge.importerIdentity != importerIdentity) continue;
        const LocalModuleSemanticRecord *record = module(edge.importedIdentity);
        if (record == nullptr) continue;

        for (const ModuleExportSymbol &exported : record->exports) {
            const std::string importedName = edge.importSpec.alias.empty()
                ? exported.name
                : edge.importSpec.alias + "." + exported.name;
            if (!seen.insert(importedName).second) continue;
            environment.importedSymbols.push_back(
                SemanticExternalSymbol{importedName,
                                       exported.kind,
                                       exported.declaration});
        }
    }
    return environment;
}

LocalModuleSemanticIndex buildLocalModuleSemanticIndex(
    LocalModuleResolver resolver,
    std::string entryIdentity,
    const std::vector<vietvm::frontend::AstImportSpec> &rootImports,
    ModuleIndexMode mode) {
    if (mode == ModuleIndexMode::DirectOnly) {
        LocalModuleSemanticIndex index;
        index.entryIdentity = entryIdentity;
        index.graph.entryIdentity = entryIdentity;
        std::unordered_set<std::string> loaded;

        for (const auto &candidate : rootImports) {
            const LocalModuleLocation location = resolver.resolve(candidate);
            if (!fs::exists(location.path)) continue;

            LocalModuleEdge edge;
            edge.importerIdentity = entryIdentity;
            edge.importedIdentity = location.identity;
            edge.importSpec = candidate;
            if (!loaded.insert(location.identity).second) {
                edge.action = LocalModuleEdgeAction::DuplicateNoOp;
                index.graph.edges.push_back(std::move(edge));
                continue;
            }

            edge.action = LocalModuleEdgeAction::Load;
            index.graph.edges.push_back(std::move(edge));
            LocalModuleSource source = resolver.read(location);
            const vietvm::frontend::AstProgram program = parseModuleSource(source.source);
            index.modules.push_back(
                LocalModuleSemanticRecord{source.path,
                                          source.identity,
                                          moduleExports(program)});
            index.graph.modules.push_back(std::move(source));
        }
        return index;
    }

    const LocalModuleResolver scanResolver = resolver;
    LocalModuleGraphBuilder graphBuilder(
        std::move(resolver),
        [scanResolver](const std::string &source, const fs::path &) {
            std::vector<vietvm::frontend::AstImportSpec> resolved;
            for (const auto &candidate :
                 structuredLocalImports(parseModuleSource(source))) {
                if (fs::exists(scanResolver.resolve(candidate).path)) {
                    resolved.push_back(candidate);
                }
            }
            return resolved;
        });

    std::vector<vietvm::frontend::AstImportSpec> resolvedRoots;
    resolvedRoots.reserve(rootImports.size());
    for (const auto &candidate : rootImports) {
        if (fs::exists(scanResolver.resolve(candidate).path)) {
            resolvedRoots.push_back(candidate);
        }
    }

    LocalModuleSemanticIndex index;
    index.entryIdentity = entryIdentity;
    index.graph = graphBuilder.build(std::move(entryIdentity), resolvedRoots);
    index.modules.reserve(index.graph.modules.size());
    for (const LocalModuleSource &source : index.graph.modules) {
        const vietvm::frontend::AstProgram program = parseModuleSource(source.source);
        index.modules.push_back(
            LocalModuleSemanticRecord{source.path,
                                      source.identity,
                                      moduleExports(program)});
    }
    return index;
}

const char *moduleInitializationStateName(
    ModuleInitializationState state) noexcept {
    switch (state) {
        case ModuleInitializationState::Uninitialized: return "uninitialized";
        case ModuleInitializationState::Initializing: return "initializing";
        case ModuleInitializationState::Initialized: return "initialized";
        case ModuleInitializationState::Failed: return "failed";
    }
    return "unknown";
}

ModuleInitializationTracker::ModuleInitializationTracker(
    const LocalModuleSemanticIndex &index) {
    for (const LocalModuleSemanticRecord &module : index.modules) {
        states_.emplace(module.identity,
                        ModuleInitializationState::Uninitialized);
    }
}

std::optional<ModuleInitializationState> ModuleInitializationTracker::state(
    std::string_view identity) const noexcept {
    const auto found = states_.find(std::string(identity));
    if (found == states_.end()) return std::nullopt;
    return found->second;
}

bool ModuleInitializationTracker::begin(std::string_view identity) {
    const auto found = states_.find(std::string(identity));
    if (found == states_.end() ||
        found->second != ModuleInitializationState::Uninitialized) {
        return false;
    }
    found->second = ModuleInitializationState::Initializing;
    return true;
}

bool ModuleInitializationTracker::complete(std::string_view identity) {
    const auto found = states_.find(std::string(identity));
    if (found == states_.end() ||
        found->second != ModuleInitializationState::Initializing) {
        return false;
    }
    found->second = ModuleInitializationState::Initialized;
    return true;
}

bool ModuleInitializationTracker::fail(std::string_view identity) {
    const auto found = states_.find(std::string(identity));
    if (found == states_.end() ||
        found->second != ModuleInitializationState::Initializing) {
        return false;
    }
    found->second = ModuleInitializationState::Failed;
    return true;
}

} // namespace vietvm::compiler
