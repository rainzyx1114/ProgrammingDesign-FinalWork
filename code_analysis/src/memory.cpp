#include "memory.h"

StackFrame::StackFrame(const std::string& name)
    : functionName(name), lineNumber(0), previousFrame(nullptr) {
}

Value StackFrame::getVariable(const std::string& name) const {
    // Implementation
    return Value();
}

void StackFrame::setVariable(const std::string& name, const Value& val) {
    // Implementation
}

bool StackFrame::hasVariable(const std::string& name) const {
    // Implementation
    return false;
}

Memory::Memory()
    : nextObjectId(0) {
}

void Memory::pushFrame(const std::string& functionName) {
    // Implementation
}

void Memory::popFrame() {
    // Implementation
}

std::shared_ptr<StackFrame> Memory::currentFrame() {
    // Implementation
    return nullptr;
}

const std::vector<std::shared_ptr<StackFrame>>& Memory::getCallStack() const {
    return callStack;
}

Value Memory::getVariable(const std::string& name) {
    // Implementation
    return Value();
}

void Memory::setVariable(const std::string& name, const Value& val) {
    // Implementation
}

std::shared_ptr<Object> Memory::createObject(const std::string& className) {
    // Implementation
    return std::make_shared<Object>(className);
}

std::shared_ptr<Object> Memory::getObject(const std::string& objectId) {
    // Implementation
    return nullptr;
}

const std::map<std::string, std::shared_ptr<Object>>& Memory::getHeap() const {
    return heap;
}
