#include "executor.h"

Executor::Executor(std::shared_ptr<Memory> mem,
                   std::shared_ptr<SymbolTable> sym,
                   std::shared_ptr<TypeSystem> types,
                   std::shared_ptr<ClassModel> cls)
    : memory(mem), symbolTable(sym), typeSystem(types), classModel(cls),
      mode(ExecutionMode::PAUSED), shouldBreak(false), shouldContinue(false),
      shouldReturn(false), nextLineToExecute(0) {
}

void Executor::executeProgram(const std::shared_ptr<Program>& program) {
    // Implementation
}

void Executor::executeStatement(const std::shared_ptr<Stmt>& stmt) {
    // Implementation
}

Value Executor::evaluateExpression(const std::shared_ptr<Expr>& expr) {
    // Implementation
    return Value();
}

void Executor::stepInto() {
    // Implementation
}

void Executor::stepOver() {
    // Implementation
}

void Executor::stepOut() {
    // Implementation
}

void Executor::runUntilBreakpoint(int line) {
    // Implementation
}

void Executor::executeExprStmt(const std::shared_ptr<ExprStmt>& stmt) {
    // Implementation
}

void Executor::executeBlock(const std::shared_ptr<Block>& stmt) {
    // Implementation
}

void Executor::executeIfStmt(const std::shared_ptr<IfStmt>& stmt) {
    // Implementation
}

void Executor::executeWhileStmt(const std::shared_ptr<WhileStmt>& stmt) {
    // Implementation
}

void Executor::executeForStmt(const std::shared_ptr<ForStmt>& stmt) {
    // Implementation
}

void Executor::executeReturnStmt(const std::shared_ptr<ReturnStmt>& stmt) {
    // Implementation
}

Value Executor::evaluateBinaryOp(const std::shared_ptr<BinaryOp>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateUnaryOp(const std::shared_ptr<UnaryOp>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateLiteral(const std::shared_ptr<Literal>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateVariable(const std::shared_ptr<Variable>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateFunctionCall(const std::shared_ptr<FunctionCall>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateMemberAccess(const std::shared_ptr<MemberAccess>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateArrayAccess(const std::shared_ptr<ArrayAccess>& expr) {
    // Implementation
    return Value();
}

Value Executor::evaluateAssignment(const std::shared_ptr<Assignment>& expr) {
    // Implementation
    return Value();
}

bool Executor::isTrue(const Value& val) const {
    // Implementation
    return false;
}

Value Executor::applyBinaryOp(const std::string& op, const Value& left, const Value& right) {
    // Implementation
    return Value();
}

Value Executor::applyUnaryOp(const std::string& op, const Value& val) {
    // Implementation
    return Value();
}
