#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "vpp/runtime/vm.h"

// Internal VM execution fixture shared by production runtime code and tests.
// It intentionally exposes handler-level state while VM's public API stays small.
class VMRuntimeFixture {
public:
    explicit VMRuntimeFixture(VM &vm) : vm_(vm) {}

    void setOutputSink(VM::OutputSink sink) { vm_.setOutputSink(std::move(sink)); }

    void push(const StackValue &value) { vm_.stack.push_back(value); }
    const std::vector<StackValue> &stack() const { return vm_.stack; }

    const StackValue &top() const {
        if (vm_.stack.empty()) throw std::logic_error("VM fixture stack is empty");
        return vm_.stack.back();
    }

    std::size_t pc() const { return vm_.pc; }
    void setPc(std::size_t value) { vm_.pc = value; }

    bool hasVariable(int id) const { return vm_.variables.count(id) != 0; }

    const StackValue &variable(int id) const {
        const auto it = vm_.variables.find(id);
        if (it == vm_.variables.end()) throw std::logic_error("VM fixture variable is missing");
        return it->second;
    }

    void pushCallFrame(const CallFrame &frame) { vm_.callStack.push_back(frame); }

    const CallFrame &currentCallFrame() const {
        if (vm_.callStack.empty()) throw std::logic_error("VM fixture call stack is empty");
        return vm_.callStack.back();
    }

    std::size_t switchDepth() const { return vm_.switchStack.size(); }
    bool switchSkipping() const {
        if (vm_.switchStack.empty()) throw std::logic_error("VM fixture switch stack is empty");
        return vm_.switchStack.back().skippingCase;
    }
    bool switchMatched() const {
        if (vm_.switchStack.empty()) throw std::logic_error("VM fixture switch stack is empty");
        return vm_.switchStack.back().caseMatched;
    }

    int blockDepth() const { return vm_.blockDepth; }
    std::size_t tryDepth() const { return vm_.tryStack.size(); }

    void executeCall(const Instruction &instruction) { vm_.executeCallOpcode(instruction); }
    void executeValue(const Instruction &instruction) { vm_.executeValueOpcode(instruction); }
    void executeIndex(const Instruction &instruction) { vm_.executeIndexOpcode(instruction); }
    void executeObject(const Instruction &instruction) { vm_.executeObjectOpcode(instruction); }
    void executeVariable(const Instruction &instruction) { vm_.executeVariableOpcode(instruction); }
    bool executeSwitch(const Instruction &instruction) { return vm_.executeSwitchOpcode(instruction); }
    void executeLoopControl(const Instruction &instruction) { vm_.executeLoopControlOpcode(instruction); }
    void executeBlock(const Instruction &instruction) { vm_.executeBlockOpcode(instruction); }
    bool executeException(const Instruction &instruction) { return vm_.executeExceptionOpcode(instruction); }
    bool executeBranch(const Instruction &instruction) { return vm_.executeBranchOpcode(instruction); }
    void executeOutput(const Instruction &instruction) { vm_.executeOutputOpcode(instruction); }

private:
    VM &vm_;
};
