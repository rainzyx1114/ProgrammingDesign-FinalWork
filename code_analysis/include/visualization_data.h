#ifndef VISUALIZATION_DATA_H
#define VISUALIZATION_DATA_H

#include <string>
#include <vector>
#include <memory>
#include <map>

struct VariableInfo {
    std::string name;
    std::string type;
    std::string value;
    bool isPointer;
    std::string pointsTo;
};

struct StackFrameView {
    std::string functionName;
    int lineNumber;
    std::vector<VariableInfo> variables;
};

struct StackTraceView {
    std::vector<StackFrameView> frames;
};

struct MemberInfo {
    std::string name;
    std::string type;
    std::string value;
    bool isMethod;
};

struct ObjectView {
    std::string objectId;
    std::string className;
    std::string baseClass;
    std::vector<MemberInfo> members;
    std::map<std::string, std::string> vtable;
};

struct ExecutionState {
    bool isRunning;
    bool isPaused;
    int currentLine;
    std::string currentFunction;
    StackTraceView stackTrace;
    std::vector<ObjectView> objectsOnHeap;
    std::string executionLog;
};

#endif
