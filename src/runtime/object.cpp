#include "vpp/runtime/object.h"

#include <unordered_set>
#include <utility>

namespace vietvm::runtime {

ClassHandle createClass(std::string name, ClassHandle superclass) {
    auto klass = std::make_shared<RuntimeClass>();
    klass->name = std::move(name);
    klass->superclass = std::move(superclass);
    return klass;
}

bool defineMethod(const ClassHandle &klass, std::string name, int functionId) {
    if (klass == nullptr || name.empty() || functionId < 0) return false;
    klass->methods[std::move(name)] = functionId;
    return true;
}

std::optional<int> lookupMethod(const ClassHandle &klass, std::string_view name) {
    std::unordered_set<const RuntimeClass *> visited;
    for (ClassHandle current = klass; current != nullptr; current = current->superclass) {
        if (!visited.insert(current.get()).second) return std::nullopt;
        const auto found = current->methods.find(std::string(name));
        if (found != current->methods.end()) return found->second;
    }
    return std::nullopt;
}

bool isSubclassOf(const ClassHandle &klass, const ClassHandle &candidateBase) {
    if (klass == nullptr || candidateBase == nullptr) return false;
    std::unordered_set<const RuntimeClass *> visited;
    for (ClassHandle current = klass; current != nullptr; current = current->superclass) {
        if (!visited.insert(current.get()).second) return false;
        if (current == candidateBase) return true;
    }
    return false;
}

InstanceHandle createInstance(ClassHandle klass) {
    if (klass == nullptr) return nullptr;
    auto instance = std::make_shared<RuntimeInstance>();
    instance->klass = std::move(klass);
    return instance;
}

bool setInstanceField(const InstanceHandle &instance,
                      std::string name,
                      StackValue value) {
    if (instance == nullptr || name.empty()) return false;
    instance->fields[std::move(name)] = std::move(value);
    return true;
}

std::optional<StackValue> getInstanceField(const InstanceHandle &instance,
                                           std::string_view name) {
    if (instance == nullptr) return std::nullopt;
    const auto found = instance->fields.find(std::string(name));
    if (found == instance->fields.end()) return std::nullopt;
    return found->second;
}

bool hasInstanceField(const InstanceHandle &instance, std::string_view name) {
    if (instance == nullptr) return false;
    return instance->fields.find(std::string(name)) != instance->fields.end();
}

} // namespace vietvm::runtime
