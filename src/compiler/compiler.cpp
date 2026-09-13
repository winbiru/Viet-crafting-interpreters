// Compiler.cpp

#include "vm/instruction.h"
#include <filesystem>
#include <unordered_map>
#include <string>
#include "frontend/lexer.h"
#include "compiler/compileRegistry.h"
#include "common/storeString.h"
#include "vpp/compiler/pipeline.h"
#include "vpp/core/project_layout.h"

namespace vietvm::compiler {

namespace {

class CompilationRegistryBinding {
public:
    explicit CompilationRegistryBinding(CompilationRegistryState &state)
        : previous_(setActiveCompilationRegistryState(&state)) {}

    ~CompilationRegistryBinding() {
        setActiveCompilationRegistryState(previous_);
    }

    CompilationRegistryBinding(const CompilationRegistryBinding &) = delete;
    CompilationRegistryBinding &operator=(const CompilationRegistryBinding &) = delete;

private:
    CompilationRegistryState *previous_;
};

thread_local std::size_t pipelineDepth = 0;

class PipelineDepthGuard {
public:
    PipelineDepthGuard() : topLevel_(pipelineDepth++ == 0) {}
    ~PipelineDepthGuard() { --pipelineDepth; }

    bool topLevel() const noexcept { return topLevel_; }

private:
    bool topLevel_ = false;
};

void clearTransientCompilationState() {
    clearImportedFiles();
    clearClassAccessState();
}

std::vector<vietvm::frontend::AstImportSpec> directLocalSourceImports(
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
        if (vietvm::core::utf8Path(statement.importSpec.target).extension() != ".vi") {
            continue;
        }
        imports.push_back(statement.importSpec);
    }
    return imports;
}

} // namespace

void resetCompilationState() {
    StringPool::clear();
    clearTransientCompilationState();
    hamMap::bytecodeMap().clear();
    hamMap::clearHamNameIndexMap();
    hamMap::resetHamIdCounter();
}

void CompilationContext::clear() {
    CompilationRegistryState::clear();
}

} // namespace vietvm::compiler

namespace vietvm::compiler {

CompilationArtifacts compilePipeline(
    const std::string &source,
    const std::unordered_map<std::string, Opcode> &keywordMap,
    bool emitMainCall) {
    PipelineDepthGuard depthGuard;
    CompilationArtifacts artifacts;

    // Lexer normalization (including multi-word keywords) remains part of the
    // lexing stage, and preserves source spans when tokens are merged.
    artifacts.tokens = postProcessTokensWithSpans(tokenizeWithSpans(source));
    artifacts.ast = vietvm::frontend::parseTokens(artifacts.tokens);

    SemanticEnvironment semanticEnvironment;
    const auto localImports = directLocalSourceImports(artifacts.ast);
    if (depthGuard.topLevel() && !localImports.empty()) {
        namespace fs = std::filesystem;
        fs::path resolutionBase = activeCompilationRegistryState().importResolutionBase;
        if (resolutionBase.empty()) resolutionBase = fs::current_path();
        constexpr std::string_view kEntryIdentity = "entry://current-compilation";
        artifacts.moduleIndex = buildLocalModuleSemanticIndex(
            LocalModuleResolver(resolutionBase),
            std::string(kEntryIdentity),
            localImports,
            ModuleIndexMode::DirectOnly);
        semanticEnvironment = artifacts.moduleIndex->semanticEnvironmentFor(
            kEntryIdentity);
    }
    artifacts.semantic = analyzeSemantics(
        artifacts.ast, semanticEnvironment, ResolutionPolicy::PreserveLegacy);

    for (const SemanticDiagnostic &diagnostic : artifacts.semantic.diagnostics) {
        if (diagnostic.severity == SemanticDiagnosticSeverity::Error) {
            throw std::runtime_error(diagnostic.message);
        }
    }

    artifacts.ir = lowerToIr(artifacts.ast, artifacts.semantic);
    artifacts.optimization = optimizeIr(artifacts.ir);

    const DirectIrSupport directSupport = analyzeDirectIrSupport(artifacts.ir);
    artifacts.unsupportedDirectIrRegions = directSupport.unsupportedRegions;
    artifacts.bytecode = emitDirectBytecode(
        artifacts.ir, keywordMap, emitMainCall);
    return artifacts;
}

CompilationArtifacts compilePipeline(
    CompilationContext &context,
    const std::string &source,
    const std::unordered_map<std::string, Opcode> &keywordMap,
    bool emitMainCall) {
    namespace fs = std::filesystem;

    if (context.importResolutionBase.empty()) {
        context.importResolutionBase = fs::current_path();
    } else {
        try {
            context.importResolutionBase =
                fs::absolute(context.importResolutionBase).lexically_normal();
        } catch (...) {
            context.importResolutionBase = context.importResolutionBase.lexically_normal();
        }
    }

    context.clear();
    CompilationRegistryBinding registryBinding(context);
    resetCompilationState();

    try {
        CompilationArtifacts artifacts =
            compilePipeline(source, keywordMap, emitMainCall);

        // StringPool/function state is already owned by `context`; only transient
        // import/access-control state remains thread-local during compilation.
        clearTransientCompilationState();
        return artifacts;
    } catch (...) {
        context.clear();
        clearTransientCompilationState();
        throw;
    }
}

} // namespace vietvm::compiler

std::vector<Instruction> compileSource(const std::string& source,
                                       const std::unordered_map<std::string,Opcode>& keywordMap,
                                       bool emitMainCall)
{
    return vietvm::compiler::compilePipeline(source, keywordMap, emitMainCall).bytecode;
}
