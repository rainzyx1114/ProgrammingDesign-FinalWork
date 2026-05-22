#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "value.h"

class StackFrame {
public:
    std::string functionName;
    int lineNumber;
    std::shared_ptr<StackFrame> previousFrame;
    
    StackFrame(const std::string& name, std::shared_ptr<StackFrame> pre);
};

class Memory {
private:
    std::vector<std::shared_ptr<StackFrame>> callStack; // function call stack
    // lexical frames: each element is a vector of slots (Values) for that lexical depth
    std::vector<std::vector<Value>> lexicalFrames;
    // Save lexical frames when entering function calls, so we can restore after returning
    std::vector<std::vector<std::vector<Value>>> lexicalFrameHistory;
    std::map<std::string, std::shared_ptr<Object>> heap;  // Simple object management
    int nextObjectId;

public:
    Memory();
    
    // Stack operations (function calls)
    void pushFrame(const std::string& functionName);
    void popFrame();
    std::shared_ptr<StackFrame> currentFrame();
    const std::vector<std::shared_ptr<StackFrame>>& getCallStack() const;
    
    // Lexical scope operations
    void initLexicalFrames(const std::vector<int>& slotsPerLevel);
    void pushScopeFrame(int slots);
    void popScopeFrame();
    int getCurrentLexicalDepth() const { return (int)lexicalFrames.size() - 1; }


    // Variable access by binding (depth, slot)
    Value getByBinding(int binding_depth, int slot_index);
    void setByBinding(int binding_depth, int slot_index, const Value& val);

    // Expose current lexical slots for visualization
    std::vector<Value> getCurrentLexicalSlots() const;

    // Object (heap) operations
    std::shared_ptr<Object> createObject(const std::string& className);
    std::shared_ptr<Object> getObject(const std::string& objectId);
    const std::map<std::string, std::shared_ptr<Object>>& getHeap() const;
    
    // Frame utilities
    int getFrameCount() const { return callStack.size(); }
};

#endif
