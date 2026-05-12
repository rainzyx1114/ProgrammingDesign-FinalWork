#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "memory.h"
#include "symbol_table.h"
#include "type_system.h"
#include "executor.h"
#include "class_model.h"
#include "visualization_data.h"

class CodeAnalyzer {
private:
    std::shared_ptr<Lexer> lexer;
    std::shared_ptr<Parser> parser;
    std::shared_ptr<Memory> memory;
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<TypeSystem> typeSystem;
    std::shared_ptr<ClassModel> classModel;
    std::shared_ptr<Executor> executor;
    
    std::shared_ptr<Program> program;
    bool isLoaded;
    bool isExecuting;
    
public:
    CodeAnalyzer();
    
    // Code loading and parsing
    bool loadCode(const std::string& sourceCode);
    std::string getParseError() const;
    
    // Execution control
    void start();
    void stepExecute();  // Single step
    void runContinuously();
    void pause();
    void stop();
    bool isRunning() const { return isExecuting; }
    
    // State queries
    ExecutionState getExecutionState();
    StackTraceView getStackTrace();
    std::vector<VariableInfo> getVariables();
    std::vector<ObjectView> getObjectsOnHeap();
    
    // Query current position
    int getCurrentLine() const;
    std::string getCurrentFunction() const;
};

#endif
