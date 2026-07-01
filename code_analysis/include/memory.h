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
    std::vector<std::vector<std::string>> lexicalVariableNames;
    
    StackFrame(const std::string& name, std::shared_ptr<StackFrame> pre,
               const std::vector<std::vector<std::string>>& names = {});
};

class Memory {
private:
    std::vector<std::shared_ptr<StackFrame>> callStack; // function call stack
    // lexical frames: each element is a vector of slots (Values) for that lexical depth
    std::vector<std::vector<Value>> lexicalFrames;
    std::vector<std::vector<std::string>> lexicalNameFrames;
    std::vector<std::vector<std::string>> lexicalTypeFrames;
    // Save lexical frames when entering function calls, so we can restore after returning
    std::vector<std::vector<std::vector<Value>>> lexicalFrameHistory;
    std::vector<std::vector<std::vector<std::string>>> lexicalNameFrameHistory;
    std::vector<std::vector<std::vector<std::string>>> lexicalTypeFrameHistory;
    std::map<std::string, std::shared_ptr<Object>> heap;  // Simple object management
    int nextObjectId;

public:
    Memory();
    
    // Stack operations (function calls)
    void pushFrame(const std::string& functionName,
                   const std::vector<std::vector<std::string>>& variableNames = {});
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
    std::vector<std::vector<Value>> getLexicalFrames() const;
    std::vector<std::vector<std::string>> getLexicalVariableNames() const;
    std::vector<std::vector<std::string>> getLexicalVariableTypes() const;
    std::vector<std::vector<Value>> getLexicalFramesForCallFrame(int frameIndex) const;
    std::vector<std::vector<std::string>> getLexicalVariableNamesForCallFrame(int frameIndex) const;
    std::vector<std::vector<std::string>> getLexicalVariableTypesForCallFrame(int frameIndex) const;
    void setLexicalVariableName(int binding_depth, int slot_index, const std::string& name);
    void setLexicalVariableType(int binding_depth, int slot_index, const std::string& typeName);
    std::string getLexicalVariableType(int binding_depth, int slot_index) const;

    // Object (heap) operations
    int createObjectReturnId(const std::string& className);
    int putOnHeap(std::shared_ptr<Object> obj);
    std::shared_ptr<Object> getObject(const std::string& objectId);
    std::shared_ptr<Object> getObjectById(int id);
    const std::map<std::string, std::shared_ptr<Object>>& getHeap() const;

    // Array operations
    std::shared_ptr<Object> createArray(int size);
    
    // Frame utilities
    int getFrameCount() const { return callStack.size(); }
};

#endif
