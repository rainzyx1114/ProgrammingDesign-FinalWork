#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <memory>
#include <string>
#include <functional>
#include <stack>
#include "ast.h"
#include "memory.h"
#include "symbol_table.h"
#include "type_system.h"
#include "value.h"
#include "visualization_data.h"

class ClassModel;

enum class ExecutionMode {
    RUN,           // Run continuously
    STEP,          // Single step
    PAUSED
};

class ExecutorVisitor : public Visitor {
private:
    std::shared_ptr<Memory> memory;
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<TypeSystem> typeSystem;
    std::shared_ptr<ClassModel> classModel;
    
    ExecutionMode mode;
    bool shouldBreak;
    bool shouldContinue;
    bool shouldReturn;
    Value returnValue;
    int nextLineToExecute;
    
    // Execution stack for nested calls
    std::stack<std::shared_ptr<ASTNode>> callStack;
    
public:
    ExecutorVisitor(std::shared_ptr<Memory> mem,
                   std::shared_ptr<SymbolTable> sym,
                   std::shared_ptr<TypeSystem> types,
                   std::shared_ptr<ClassModel> cls);
    
    // Visitor methods for statements
    void visit(ExprStmt& node) override;
    void visit(Block& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    
    // Visitor methods for expressions (return values)
    void visit(BinaryOp& node) override;
    void visit(LogicalOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(Literal& node) override;
    void visit(Variable& node) override;
    void visit(FunctionCall& node) override;
    void visit(MemberAccess& node) override;
    void visit(ArrayAccess& node) override;
    void visit(Assignment& node) override;
    
    // Visitor methods for declarations
    void visit(VarDecl& node) override;
    void visit(FuncDecl& node) override;
    void visit(ClassDecl& node) override;
    void visit(Program& node) override;
    
    // Public interface methods
    void executeProgram(const std::shared_ptr<Program>& program);
    Value evaluateExpression(const std::shared_ptr<Expr>& expr);
    
    // Single step execution
    // void stepInto();
    // void stepOver();
    // void stepOut();
    // void runUntilBreakpoint(int line);
    
    // Query execution state
    std::shared_ptr<Memory> getMemory() const { return memory; }
    ExecutionMode getMode() const { return mode; }
    int getNextLine() const { return nextLineToExecute; }
    bool hasReturnValue() const { return shouldReturn; }
    Value getReturnValue() const { return returnValue; }
    
private:
    // Helper functions
    bool isTrue(const Value& val) const;
    Value applyBinaryOp(const std::string& op, const Value& left, const Value& right);
    Value applyUnaryOp(const std::string& op, const Value& val);
    std::vector<std::vector<std::string>> buildCurrentLexicalVariableNames() const;
    std::vector<VariableInfo> buildVariableInfoForCallFrame(int frameIndex,
                                                           const std::vector<std::vector<Value>>& lexicalFrames,
                                                           const std::vector<std::vector<std::string>>& lexicalNames) const;
    
    // Current evaluation result (for expressions)
    Value currentValue;

    // Execution trace collection
    std::vector<Stepsnapshot> executionTrace;
    void recordSnapshot(const std::string& event, int lineNumber = 0);

public:
    // Expose execution trace
    std::vector<Stepsnapshot> getExecutionTrace() const { return executionTrace; }
};

class Executor {
private:
    std::shared_ptr<ExecutorVisitor> visitor;
    
public:
    Executor(std::shared_ptr<Memory> mem,
             std::shared_ptr<SymbolTable> sym,
             std::shared_ptr<TypeSystem> types,
             std::shared_ptr<ClassModel> cls);
    
    // Execution control
    void executeProgram(const std::shared_ptr<Program>& program);
    Value evaluateExpression(const std::shared_ptr<Expr>& expr);
    
    // Single step execution
    // void stepInto();
    // void stepOver();
    // void stepOut();
    // void runUntilBreakpoint(int line);
    
    // Query execution state
    std::shared_ptr<Memory> getMemory() const;
    ExecutionMode getMode() const;
    int getNextLine() const;

    // Execution trace access
    std::vector<Stepsnapshot> getExecutionTrace() const;
};

#endif
