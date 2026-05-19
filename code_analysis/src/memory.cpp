#include "memory.h"

StackFrame::StackFrame(const std::string& name, std::shared_ptr<StackFrame> pre)
    : functionName(name), lineNumber(0), previousFrame(pre) {
}

Value StackFrame::getVariable(const std::string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) return it->second;
    return Value();
}

void StackFrame::setVariable(const std::string& name, const Value& val) {
    variables[name] = val;
}

bool StackFrame::hasVariable(const std::string& name) const {
    return variables.find(name) != variables.end();
}

Memory::Memory()
    : nextObjectId(0) {
}

void Memory::pushFrame(const std::string& functionName) {
    std::shared_ptr<StackFrame> prev = nullptr;
    if (!callStack.empty()) prev = callStack.back();
    auto frame = std::make_shared<StackFrame>(functionName, prev);
    callStack.push_back(frame);
}

void Memory::popFrame() {
    if (!callStack.empty()) {
        callStack.pop_back();
    }
}

std::shared_ptr<StackFrame> Memory::currentFrame() {
    if (callStack.empty()) {
        return nullptr;
    }
    return callStack.back();
}

const std::vector<std::shared_ptr<StackFrame>>& Memory::getCallStack() const {
    return callStack;
}

Value Memory::getVariable(const std::string& name) {
    
    return Value();
}

void Memory::setVariable(const std::string& name, const Value& val) {
    auto frame = currentFrame();
    if (frame) frame->setVariable(name, val);
}

std::shared_ptr<Object> Memory::createObject(const std::string& className) {
    auto obj = std::make_shared<Object>(className);
    std::string id = "obj" + std::to_string(nextObjectId++);
    heap[id] = obj;
    return obj;
}

std::shared_ptr<Object> Memory::getObject(const std::string& objectId) {
    auto it = heap.find(objectId);
    if (it != heap.end()) return it->second;
    return nullptr;
}

const std::map<std::string, std::shared_ptr<Object>>& Memory::getHeap() const {
    return heap;
}
