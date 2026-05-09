#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <memory>
#include <string>
#include <functional>
#include "ast.h"
#include "memory.h"
#include "symbol_table.h"
#include "type_system.h"
#include "value.h"

class ClassModel;

enum class ExecutionMode {
    RUN,           // Run continuously
    STEP,          // Single step
    PAUSED
};

class Executor {
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
    
public:
    Executor(std::shared_ptr<Memory> mem,
             std::shared_ptr<SymbolTable> sym,
             std::shared_ptr<TypeSystem> types,
             std::shared_ptr<ClassModel> cls);
    
    // Execution control
    void executeProgram(const std::shared_ptr<Program>& program);
    void executeStatement(const std::shared_ptr<Stmt>& stmt);
    Value evaluateExpression(const std::shared_ptr<Expr>& expr);
    
    // Single step execution
    void stepInto();
    void stepOver();
    void stepOut();
    void runUntilBreakpoint(int line);
    
    // Query execution state
    std::shared_ptr<Memory> getMemory() const { return memory; }
    ExecutionMode getMode() const { return mode; }
    int getNextLine() const { return nextLineToExecute; }
    
private:
    // Statement execution
    void executeExprStmt(const std::shared_ptr<ExprStmt>& stmt);
    void executeBlock(const std::shared_ptr<Block>& stmt);
    void executeIfStmt(const std::shared_ptr<IfStmt>& stmt);
    void executeWhileStmt(const std::shared_ptr<WhileStmt>& stmt);
    void executeForStmt(const std::shared_ptr<ForStmt>& stmt);
    void executeReturnStmt(const std::shared_ptr<ReturnStmt>& stmt);
    
    // Expression evaluation
    Value evaluateBinaryOp(const std::shared_ptr<BinaryOp>& expr);
    Value evaluateUnaryOp(const std::shared_ptr<UnaryOp>& expr);
    Value evaluateLiteral(const std::shared_ptr<Literal>& expr);
    Value evaluateVariable(const std::shared_ptr<Variable>& expr);
    Value evaluateFunctionCall(const std::shared_ptr<FunctionCall>& expr);
    Value evaluateMemberAccess(const std::shared_ptr<MemberAccess>& expr);
    Value evaluateArrayAccess(const std::shared_ptr<ArrayAccess>& expr);
    Value evaluateAssignment(const std::shared_ptr<Assignment>& expr);
    
    // Helper functions
    bool isTrue(const Value& val) const;
    Value applyBinaryOp(const std::string& op, const Value& left, const Value& right);
    Value applyUnaryOp(const std::string& op, const Value& val);
};

#endif
