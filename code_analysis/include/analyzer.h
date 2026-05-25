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

// Concrete visitor for analyzing AST
class ASTAnalyzer : public Visitor {
private:
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<TypeSystem> typeSystem;
    std::shared_ptr<ClassModel> classModel;

public:
    ASTAnalyzer(std::shared_ptr<SymbolTable> st, std::shared_ptr<TypeSystem> ts,
                std::shared_ptr<ClassModel> cm);

    // CodeAnalyzer will set executor later
    friend class CodeAnalyzer;

    void visit(BinaryOp& node) override;
    void visit(LogicalOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(Literal& node) override;
    void visit(Variable& node) override;
    void visit(FunctionCall& node) override;
    void visit(MemberAccess& node) override;
    void visit(ArrayAccess& node) override;
    void visit(Assignment& node) override;
    void visit(ExprStmt& node) override;
    void visit(Block& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(VarDecl& node) override;
    void visit(FuncDecl& node) override;
    void visit(ClassDecl& node) override;
    void visit(Program& node) override;
};

class CodeAnalyzer {
private:
    std::shared_ptr<Lexer> lexer;
    std::shared_ptr<Parser> parser;
    std::shared_ptr<Memory> memory;
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<TypeSystem> typeSystem;
    std::shared_ptr<ClassModel> classModel;
    std::shared_ptr<Executor> executor;
    std::shared_ptr<ASTAnalyzer> astAnalyzer;
    
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
    void stepExecute();
    void runContinuously();
    void pause();
    void stop();
    bool isRunning() const { return isExecuting; }
    
    // State queries
    ExecutionState getExecutionState();
    StackTraceView getStackTrace();
    std::vector<VariableInfo> getVariables();
    std::vector<ObjectView> getObjectsOnHeap();
    std::vector<Stepsnapshot> getExecutionTrace();
    
    // Query current position
    int getCurrentLine() const;
    std::string getCurrentFunction() const;
};

#endif
