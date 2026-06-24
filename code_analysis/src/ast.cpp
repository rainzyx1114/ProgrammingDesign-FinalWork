#include "ast.h"

// Expression nodes
void BinaryOp::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void LogicalOp::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void UnaryOp::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Literal::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Variable::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void FunctionCall::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void MemberAccess::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void ArrayAccess::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void NewExpr::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Assignment::accept(Visitor& visitor) {
    visitor.visit(*this);
}

// Statement nodes
void ExprStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Block::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void IfStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void WhileStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void ForStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void ReturnStmt::accept(Visitor& visitor) {
    visitor.visit(*this);
}

// Declaration nodes
void VarDecl::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void FuncDecl::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void ClassDecl::accept(Visitor& visitor) {
    visitor.visit(*this);
}

void Program::accept(Visitor& visitor) {
    visitor.visit(*this);
}