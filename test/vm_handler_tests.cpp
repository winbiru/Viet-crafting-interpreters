#include <iostream>
#include <string>
#include <vector>

#include "common/vm_native_stdlib_helpers.h"
#include "vpp/runtime/vm_fixture.h"

namespace {

int failures = 0;

void fail(const std::string &name, const std::string &detail) {
    std::cerr << "FAIL: " << name << ": " << detail << '\n';
    ++failures;
}

void expect(bool condition, const std::string &name, const std::string &detail) {
    if (!condition) fail(name, detail);
}

int asInt(const StackValue &value, const std::string &name) {
    if (!std::holds_alternative<int>(value)) {
        fail(name, "expected integer value");
        return 0;
    }
    return std::get<int>(value);
}

std::string asString(const StackValue &value, const std::string &name) {
    if (!std::holds_alternative<std::string>(value)) {
        fail(name, "expected string value");
        return {};
    }
    return std::get<std::string>(value);
}

Instruction instruction(Opcode op, int operand = 0, int operandIndex = 0, int operandValue = 0) {
    return {op, operand, operandIndex, operandValue};
}

void testValueHandlerState() {
    VM vm({}, {});
    VMRuntimeFixture access(vm);
    access.push(make_int_value(7));
    access.push(make_int_value(5));
    access.executeValue(instruction(OP_CONG));

    expect(access.stack().size() == 1, "value handler", "binary operation must consume two values");
    expect(asInt(access.top(), "value handler") == 12,
           "value handler", "7 + 5 must leave 12 on the stack");
}

void testIndexHandlerState() {
    VM vm({}, {});
    VMRuntimeFixture access(vm);
    const StackValue listValue = make_list_value({make_int_value(1), make_int_value(2)});
    const ListHandle list = std::get<ListHandle>(listValue);

    access.push(listValue);
    access.push(make_int_value(1));
    access.executeIndex(instruction(OP_DOC_CHI_SO));
    expect(asInt(access.top(), "index read handler") == 2,
           "index read handler", "list[1] must produce 2");

    access.push(listValue);
    access.push(make_int_value(0));
    access.push(make_int_value(9));
    access.executeIndex(instruction(OP_GAN_CHI_SO));
    expect(asInt(list->elements[0], "index write handler") == 9,
           "index write handler", "list[0] must be mutated to 9");
}

void testVariableAndCallFrameHandlerState() {
    VM vm({}, {});
    VMRuntimeFixture access(vm);

    access.push(make_int_value(10));
    access.push(make_int_value(3));
    access.executeVariable(instruction(OP_GAN));
    expect(access.hasVariable(3), "variable handler", "assignment must create variable 3");
    expect(asInt(access.variable(3), "variable handler") == 10,
           "variable handler", "variable 3 must contain 10");

    CallFrame frame;
    frame.args.push_back(make_int_value(41));
    frame.localsIndexed = true;
    access.pushCallFrame(frame);
    access.executeVariable(instruction(OP_PARAM, 0, 0, 0));
    expect(access.currentCallFrame().localsVec.size() == 1,
           "parameter handler", "parameter binding must create local slot 0");
    expect(asInt(access.currentCallFrame().localsVec[0], "parameter handler") == 41,
           "parameter handler", "argument 0 must bind to local slot 0");
}

void testCallHandlerState() {
    VM vm({}, {});
    vm.hamBytecodeMap.emplace(7, std::vector<Instruction>{
        instruction(OP_PARAM, 0, 0, 0),
        instruction(OP_TEN_BIEN_GIA_TRI, 0, 0, 0),
        instruction(OP_BIEN_SO, 1, 0, 0),
        instruction(OP_CONG),
        instruction(OP_TRA_VE),
    });
    VMRuntimeFixture access(vm);
    access.push(make_int_value(41));
    access.executeCall(instruction(OP_GOI, 1, 7, 0));

    expect(access.stack().size() == 1, "call handler", "function return must be pushed to caller stack");
    expect(asInt(access.top(), "call handler") == 42,
           "call handler", "direct function handler must return 42");
}

void testBranchHandlerState() {
    VM vm({instruction(OP_BIEN_SO), instruction(OP_BIEN_SO), instruction(OP_DUNG_CHUONG_TRINH)}, {});
    VMRuntimeFixture access(vm);
    access.push(make_int_value(0));

    const bool jumped = access.executeBranch(instruction(OP_JUMP_IF_FALSE, 2));
    expect(jumped, "branch handler", "false condition must request a jump");
    expect(access.pc() == 2, "branch handler", "false condition must set pc to target 2");
    expect(access.stack().empty(), "branch handler", "condition must be consumed");
}

void testSwitchAndBlockHandlerState() {
    VM vm({}, {});
    VMRuntimeFixture access(vm);
    access.push(make_int_value(2));
    access.executeSwitch(instruction(OP_CHON));
    expect(access.switchDepth() == 1, "switch handler", "OP_CHON must create a switch frame");
    expect(access.switchSkipping(), "switch handler", "switch starts in skipping mode");

    access.executeSwitch(instruction(OP_CA, 2, -1, 0));
    expect(access.switchMatched(), "switch handler", "matching OP_CA must mark the frame matched");
    expect(!access.switchSkipping(), "switch handler", "matching OP_CA must enable its body");

    VM blockVm({}, {});
    VMRuntimeFixture blockAccess(blockVm);
    blockAccess.executeBlock(instruction(OP_MO_KHOI));
    expect(blockAccess.blockDepth() == 1, "block handler", "open block must increment depth");
    blockAccess.executeBlock(instruction(OP_DONG_KHOI));
    expect(blockAccess.blockDepth() == 0, "block handler", "close block must restore depth");
}

void testExceptionHandlerState() {
    VM vm(std::vector<Instruction>(5, instruction(OP_DONG_LENH)), {});
    VMRuntimeFixture access(vm);
    access.executeException(instruction(OP_THU, 4, 1, 0));
    expect(access.tryDepth() == 1, "exception handler", "OP_THU must push a try frame");

    access.push(make_string_value("boom"));
    const bool jumped = access.executeException(instruction(OP_NEM));
    expect(jumped, "exception handler", "OP_NEM with a handler must jump to catch");
    expect(access.pc() == 4, "exception handler", "throw must set pc to catch address");
    expect(asString(access.top(), "exception handler") == "boom",
           "exception handler", "thrown value must survive stack unwind");

    access.executeException(instruction(OP_BAT_LOI, 0, 1, 0));
    expect(access.hasVariable(1), "exception handler", "catch must bind the error variable");
    expect(asString(access.variable(1), "exception handler") == "boom",
           "exception handler", "catch variable must contain thrown value");
}

void testLoopControlHandlerState() {
    VM vm({
        instruction(OP_BO_QUA),
        instruction(OP_BIEN_SO, 99),
        instruction(OP_CAP_NHAT),
        instruction(OP_DUNG_CHUONG_TRINH),
    }, {});
    VMRuntimeFixture access(vm);
    access.setPc(0);
    access.executeLoopControl(instruction(OP_BO_QUA));
    expect(access.pc() == 2, "loop-control handler", "continue must target the nearest update marker");
}

void testOutputHandlerUsesSink() {
    VM vm({}, {});
    VMRuntimeFixture access(vm);
    std::string output;
    access.setOutputSink([&output](const std::string &text) { output += text; });
    access.push(make_int_value(42));
    access.executeOutput(instruction(OP_IN));

    expect(output == "[IN] 42\n",
           "output handler", "OP_IN must emit through the configured sink");
    expect(access.stack().empty(),
           "output handler", "OP_IN must consume the emitted value");
}

void testFilesystemPredicatesTreatMissingPathAsFalse() {
    StackValue result = make_null_value();
    std::string error;
    const std::vector<StackValue> args = {
        make_string_value(".tmp_vpp_path_that_must_not_exist/không-có.txt")
    };

    const bool handled = vietvm::helpers::handleNativeFoundationFunction(
        "là tệp", args, result, error);
    expect(handled, "filesystem predicate", "là tệp must be handled by foundation native layer");
    expect(error.empty(), "filesystem predicate", "a missing path must not be reported as an OS error");
    expect(asInt(result, "filesystem predicate") == 0,
           "filesystem predicate", "là tệp(missing) must return 0");
}

} // namespace

int main() {
    testValueHandlerState();
    testIndexHandlerState();
    testVariableAndCallFrameHandlerState();
    testCallHandlerState();
    testBranchHandlerState();
    testSwitchAndBlockHandlerState();
    testExceptionHandlerState();
    testLoopControlHandlerState();
    testOutputHandlerUsesSink();
    testFilesystemPredicatesTreatMissingPathAsFalse();

    if (failures != 0) {
        std::cerr << failures << " VM handler unit test(s) failed\n";
        return 1;
    }
    std::cout << "VM handler unit tests passed\n";
    return 0;
}
