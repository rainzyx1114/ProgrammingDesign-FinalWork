#include "memory.h"

StackFrame::StackFrame(const std::string& name, std::shared_ptr<StackFrame> pre,
                         const std::vector<std::vector<std::string>>& names)
    : functionName(name), lineNumber(0), previousFrame(pre), lexicalVariableNames(names) {
}

Memory::Memory()
    : nextObjectId(0) {
    // start with a global lexical frame (depth 0) with zero slots; will be resized by initLexicalFrames
    lexicalFrames.emplace_back();
    lexicalNameFrames.emplace_back();
    lexicalTypeFrames.emplace_back();
}

void Memory::pushFrame(const std::string& functionName,
                       const std::vector<std::vector<std::string>>& variableNames) {
    std::shared_ptr<StackFrame> prev = nullptr;
    if (!callStack.empty()) prev = callStack.back();
    auto frame = std::make_shared<StackFrame>(functionName, prev, variableNames);
    callStack.push_back(frame);
    // Save current lexical frames and names before entering a new function call.
    lexicalFrameHistory.push_back(lexicalFrames);
    lexicalNameFrameHistory.push_back(lexicalNameFrames);
    lexicalTypeFrameHistory.push_back(lexicalTypeFrames);
    // entering a function: reset lexical frames and names to global depth 0
    lexicalFrames.clear();
    lexicalNameFrames.clear();
    lexicalTypeFrames.clear();
    lexicalFrames.emplace_back();
    lexicalNameFrames.emplace_back();
    lexicalTypeFrames.emplace_back();
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
    if (!lexicalNameFrameHistory.empty()) {
        lexicalNameFrames = lexicalNameFrameHistory.back();
        lexicalNameFrameHistory.pop_back();
    } else {
        lexicalNameFrames.clear();
        lexicalNameFrames.emplace_back();
    }
    if (!lexicalTypeFrameHistory.empty()) {
        lexicalTypeFrames = lexicalTypeFrameHistory.back();
        lexicalTypeFrameHistory.pop_back();
    } else {
        lexicalTypeFrames.clear();
        lexicalTypeFrames.emplace_back();
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
    lexicalNameFrames.clear();
    lexicalTypeFrames.clear();
    for (int s : slotsPerLevel) {
        lexicalFrames.emplace_back(std::vector<Value>(s));
        std::vector<std::string> names;
        std::vector<std::string> types;
        for (int i = 0; i < s; ++i) {
            names.push_back("slot" + std::to_string(i));
            types.push_back("");
        }
        lexicalNameFrames.push_back(std::move(names));
        lexicalTypeFrames.push_back(std::move(types));
    }
    if (lexicalFrames.empty()) {
        lexicalFrames.emplace_back();
        lexicalNameFrames.emplace_back();
        lexicalTypeFrames.emplace_back();
    }
}

void Memory::pushScopeFrame(int slots) {
    lexicalFrames.emplace_back(std::vector<Value>(slots));
    std::vector<std::string> names;
    std::vector<std::string> types;
    for (int i = 0; i < slots; ++i) {
        names.push_back("slot" + std::to_string(i));
        types.push_back("");
    }
    lexicalNameFrames.emplace_back(std::move(names));
    lexicalTypeFrames.emplace_back(std::move(types));
}

void Memory::popScopeFrame() {
    if (!lexicalFrames.empty()) lexicalFrames.pop_back();
    if (!lexicalNameFrames.empty()) lexicalNameFrames.pop_back();
    if (!lexicalTypeFrames.empty()) lexicalTypeFrames.pop_back();
    if (lexicalFrames.empty()) lexicalFrames.emplace_back();
    if (lexicalNameFrames.empty()) lexicalNameFrames.emplace_back();
    if (lexicalTypeFrames.empty()) lexicalTypeFrames.emplace_back();
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

void Memory::setLexicalVariableName(int binding_depth, int slot_index, const std::string& name) {
    if (binding_depth < 0) return;
    if (binding_depth >= (int)lexicalNameFrames.size()) {
        lexicalNameFrames.resize(binding_depth + 1);
    }
    auto &names = lexicalNameFrames[binding_depth];
    if (slot_index < 0) return;
    if (slot_index >= (int)names.size()) names.resize(slot_index + 1);
    names[slot_index] = name;
}

void Memory::setLexicalVariableType(int binding_depth, int slot_index, const std::string& typeName) {
    if (binding_depth < 0) return;
    if (binding_depth >= (int)lexicalTypeFrames.size()) {
        lexicalTypeFrames.resize(binding_depth + 1);
    }
    auto &types = lexicalTypeFrames[binding_depth];
    if (slot_index < 0) return;
    if (slot_index >= (int)types.size()) types.resize(slot_index + 1);
    types[slot_index] = typeName;
}

std::string Memory::getLexicalVariableType(int binding_depth, int slot_index) const {
    if (binding_depth < 0 || binding_depth >= (int)lexicalTypeFrames.size()) return "";
    auto &types = lexicalTypeFrames[binding_depth];
    if (slot_index < 0 || slot_index >= (int)types.size()) return "";
    return types[slot_index];
}

int Memory::createObjectReturnId(const std::string& className) {
    auto obj = std::make_shared<Object>(className);
    int objId = nextObjectId++;
    std::string id = "obj" + std::to_string(objId);
    heap[id] = obj;
    return objId;
}

int Memory::putOnHeap(std::shared_ptr<Object> obj) {
    int objId = nextObjectId++;
    std::string id = "obj" + std::to_string(objId);
    heap[id] = obj;
    return objId;
}

std::shared_ptr<Object> Memory::getObject(const std::string& objectId) {
    auto it = heap.find(objectId);
    if (it != heap.end()) return it->second;
    return nullptr;
}

std::shared_ptr<Object> Memory::getObjectById(int id) {
    return getObject("obj" + std::to_string(id));
}

std::shared_ptr<Object> Memory::createArray(int size) {
    auto obj = std::make_shared<Object>("array");
    for (int i = 0; i < size; i++) {
        obj->setMember(std::to_string(i), Value(0));
    }
    std::string id = "obj" + std::to_string(nextObjectId++);
    heap[id] = obj;
    return obj;
}

const std::map<std::string, std::shared_ptr<Object>>& Memory::getHeap() const {
    return heap;
}

std::vector<std::vector<Value>> Memory::getLexicalFrames() const {
    return lexicalFrames;
}

std::vector<std::vector<std::string>> Memory::getLexicalVariableNames() const {
    return lexicalNameFrames;
}

std::vector<std::vector<std::string>> Memory::getLexicalVariableTypes() const {
    return lexicalTypeFrames;
}

std::vector<std::vector<Value>> Memory::getLexicalFramesForCallFrame(int frameIndex) const {
    int totalFrames = callStack.size();
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        return {};
    }
    if (frameIndex == totalFrames - 1) {
        return lexicalFrames;
    }
    int historyIndex = frameIndex + 1;
    if (historyIndex < (int)lexicalFrameHistory.size()) {
        return lexicalFrameHistory[historyIndex];
    }
    return {};
}

std::vector<std::vector<std::string>> Memory::getLexicalVariableNamesForCallFrame(int frameIndex) const {
    int totalFrames = callStack.size();
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        return {};
    }
    if (frameIndex == totalFrames - 1) {
        return lexicalNameFrames;
    }
    int historyIndex = frameIndex + 1;
    if (historyIndex < (int)lexicalNameFrameHistory.size()) {
        return lexicalNameFrameHistory[historyIndex];
    }
    return {};
}

std::vector<std::vector<std::string>> Memory::getLexicalVariableTypesForCallFrame(int frameIndex) const {
    int totalFrames = callStack.size();
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        return {};
    }
    if (frameIndex == totalFrames - 1) {
        return lexicalTypeFrames;
    }
    int historyIndex = frameIndex + 1;
    if (historyIndex < (int)lexicalTypeFrameHistory.size()) {
        return lexicalTypeFrameHistory[historyIndex];
    }
    return {};
}

std::vector<Value> Memory::getCurrentLexicalSlots() const {
    if (lexicalFrames.empty()) return {};
    int d = getCurrentLexicalDepth();
    if (d < 0 || d >= (int)lexicalFrames.size()) return {};
    return lexicalFrames[d];
}