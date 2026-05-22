#include "memory.h"

StackFrame::StackFrame(const std::string& name, std::shared_ptr<StackFrame> pre)
    : functionName(name), lineNumber(0), previousFrame(pre) {
}

Memory::Memory()
    : nextObjectId(0) {
    // start with a global lexical frame (depth 0) with zero slots; will be resized by initLexicalFrames
    lexicalFrames.emplace_back();
}

void Memory::pushFrame(const std::string& functionName) {
    std::shared_ptr<StackFrame> prev = nullptr;
    if (!callStack.empty()) prev = callStack.back();
    auto frame = std::make_shared<StackFrame>(functionName, prev);
    callStack.push_back(frame);
    // Save current lexical frames before entering a new function call.
    lexicalFrameHistory.push_back(lexicalFrames);
    // entering a function: reset lexical frames to global depth 0
    lexicalFrames.clear();
    lexicalFrames.emplace_back();
}

void Memory::popFrame() {
    if (!callStack.empty()) {
        callStack.pop_back();
    }
    // Restore previous lexical frame state after returning from function call.
    if (!lexicalFrameHistory.empty()) {
        lexicalFrames = lexicalFrameHistory.back();
        lexicalFrameHistory.pop_back();
    } else {
        lexicalFrames.clear();
        lexicalFrames.emplace_back();
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

void Memory::initLexicalFrames(const std::vector<int>& slotsPerLevel) {
    lexicalFrames.clear();
    for (int s : slotsPerLevel) {
        lexicalFrames.emplace_back(std::vector<Value>(s));
    }
    if (lexicalFrames.empty()) lexicalFrames.emplace_back();
}

void Memory::pushScopeFrame(int slots) {
    lexicalFrames.emplace_back(std::vector<Value>(slots));
}

void Memory::popScopeFrame() {
    if (!lexicalFrames.empty()) lexicalFrames.pop_back();
    if (lexicalFrames.empty()) lexicalFrames.emplace_back();
}

Value Memory::getByBinding(int binding_depth, int slot_index) {
    if (binding_depth < 0 || binding_depth >= (int)lexicalFrames.size()) return Value();
    auto &frame = lexicalFrames[binding_depth];
    if (slot_index < 0 || slot_index >= (int)frame.size()) return Value();
    return frame[slot_index];
}

void Memory::setByBinding(int binding_depth, int slot_index, const Value& val) {
    if (binding_depth < 0 || binding_depth >= (int)lexicalFrames.size()) return;
    auto &frame = lexicalFrames[binding_depth];
    if (slot_index < 0) return;
    if (slot_index >= (int)frame.size()) frame.resize(slot_index + 1);
    frame[slot_index] = val;
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

std::vector<Value> Memory::getCurrentLexicalSlots() const {
    if (lexicalFrames.empty()) return {};
    int d = getCurrentLexicalDepth();
    if (d < 0 || d >= (int)lexicalFrames.size()) return {};
    return lexicalFrames[d];
}