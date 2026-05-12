#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "lexer.h"

class Type;
class Expr;
class Stmt;
class Decl;

// Base AST node
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Expression nodes
class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
};

class BinaryOp : public Expr {
public:
    Token op;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;
    BinaryOp(std::shared_ptr<Expr> lhs, Token oper, std::shared_ptr<Expr> rhs)
        : left(std::move(lhs)), op(std::move(oper)), right(std::move(rhs)) {}
};

class UnaryOp : public Expr {
public:
    Token op;
    std::shared_ptr<Expr> operand;
    UnaryOp(Token oper, std::shared_ptr<Expr> expr)
        : op(std::move(oper)), operand(std::move(expr)) {}
};

class Literal : public Expr {
public:
    Token value;
    Literal(Token val): value(std::move(val)) {}
};

class Variable : public Expr {
public:
    Token name;
    Variable(Token varName): name(std::move(varName)) {}
};

class FunctionCall : public Expr {
public:
    std::string name;
    std::vector<std::shared_ptr<Expr>> args;
};

class MemberAccess : public Expr {
public:
    std::shared_ptr<Expr> object;
    std::string member;
    bool isPointer;
};

class ArrayAccess : public Expr {
public:
    std::shared_ptr<Expr> array;
    std::shared_ptr<Expr> index;
};

class Assignment : public Expr {
public:
    std::shared_ptr<Expr> lhs;
    std::shared_ptr<Expr> rhs;
};

// Statement nodes
class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
};

class ExprStmt : public Stmt {
public:
    std::shared_ptr<Expr> expr;
    ExprStmt(std::shared_ptr<Expr> e): expr(std::move(e)) {}
};

class Block : public Stmt {
public:
    std::vector<std::shared_ptr<Stmt>> statements;
    Block(std::vector<std::shared_ptr<Stmt>> stmts): statements(std::move(stmts)) {}
};

class IfStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> thenBranch;
    std::shared_ptr<Stmt> elseBranch;
};

class WhileStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
};

class ForStmt : public Stmt {
public:
    std::shared_ptr<Stmt> init;
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Expr> update;
    std::shared_ptr<Stmt> body;
};

class ReturnStmt : public Stmt {
public:
    std::shared_ptr<Expr> value;
};

class BreakStmt : public Stmt {
};

class ContinueStmt : public Stmt {
};

// Declaration nodes
class Decl : public ASTNode {
public:
    virtual ~Decl() = default;
};

class VarDecl : public Decl {
public:
    Token name;
    std::shared_ptr<Type> type;
    std::shared_ptr<Expr> initializer;
    VarDecl(Token varName, std::shared_ptr<Type> varType, std::shared_ptr<Expr> init)
        : name(std::move(varName)), type(std::move(varType)), initializer(std::move(init)) {}
};

class FuncDecl : public Decl {
public:
    std::string name;
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> params;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<Block> body;
};

class ClassDecl : public Decl {
public:
    std::string name;
    std::string baseClass;
    std::vector<std::shared_ptr<VarDecl>> members;
    std::vector<std::shared_ptr<FuncDecl>> methods;
};

class Program : public ASTNode {
public:
    std::vector<std::shared_ptr<Decl>> declarations;
};

#endif
