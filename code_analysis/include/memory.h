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
    std::map<std::string, Value> variables;
    std::shared_ptr<StackFrame> previousFrame;
    
    StackFrame(const std::string& name);
    
    Value getVariable(const std::string& name) const;
    void setVariable(const std::string& name, const Value& val);
    bool hasVariable(const std::string& name) const;
};

class Memory {
private:
    std::vector<std::shared_ptr<StackFrame>> callStack;
    std::map<std::string, std::shared_ptr<Object>> heap;  // Simple object management
    int nextObjectId;
    
public:
    Memory();
    
    // Stack operations
    void pushFrame(const std::string& functionName);
    void popFrame();
    std::shared_ptr<StackFrame> currentFrame();
    const std::vector<std::shared_ptr<StackFrame>>& getCallStack() const;
    
    // Variable access
    Value getVariable(const std::string& name);
    void setVariable(const std::string& name, const Value& val);
    
    // Object (heap) operations
    std::shared_ptr<Object> createObject(const std::string& className);
    std::shared_ptr<Object> getObject(const std::string& objectId);
    const std::map<std::string, std::shared_ptr<Object>>& getHeap() const;
    
    // Frame utilities
    int getFrameCount() const { return callStack.size(); }
};

#endif
