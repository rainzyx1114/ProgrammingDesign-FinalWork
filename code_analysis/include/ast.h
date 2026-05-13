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

class LogicalOp : public Expr {
public:
    Token op;
    std::shared_ptr<Expr> left;
    std::shared_ptr<Expr> right;
    LogicalOp(std::shared_ptr<Expr> lhs, Token oper, std::shared_ptr<Expr> rhs)
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
    std::shared_ptr<Expr> name;
    std::vector<std::shared_ptr<Expr>> args;
    FunctionCall(std::shared_ptr<Expr> funcName, std::vector<std::shared_ptr<Expr>> arguments)
        : name(std::move(funcName)), args(std::move(arguments)) {}
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
    Token name;
    std::shared_ptr<Expr> value;
    Assignment(Token nam, std::shared_ptr<Expr> val):name(std::move(nam)), value(std::move(val)) {}
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
    IfStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> thenbtr, std::shared_ptr<Stmt> elsebtr)
        : condition(std::move(cond)), thenBranch(std::move(thenbtr)), elseBranch(std::move(elsebtr)) {}
};

class WhileStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
    WhileStmt(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> bdy)
        : condition(std::move(cond)), body(std::move(bdy)) {}
};

class ForStmt : public Stmt {
public:
    std::shared_ptr<ASTNode> init;
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Expr> update;
    std::shared_ptr<Stmt> body;
    ForStmt(std::shared_ptr<ASTNode> initializer, std::shared_ptr<Expr> cond, std::shared_ptr<Expr> upd, std::shared_ptr<Stmt> bdy)
        : init(std::move(initializer)), condition(std::move(cond)), update(std::move(upd)), body(std::move(bdy)) {}
};

class ReturnStmt : public Stmt {
public:
    std::shared_ptr<Expr> value;
    ReturnStmt(std::shared_ptr<Expr> val): value(val) {}
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
    Token name;
    std::vector<std::pair<Token, std::shared_ptr<Type>>> params;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<Block> body;
    FuncDecl(Token funcName, std::vector<std::pair<Token, std::shared_ptr<Type>>> parameters, std::shared_ptr<Type> retType, std::shared_ptr<Block> funcBody)
        : name(std::move(funcName)), params(std::move(parameters)), returnType(std::move(retType)), body(std::move(funcBody)) {}
};

class ClassDecl : public Decl {
public:
    Token name;
    Token baseClass;
    std::vector<std::shared_ptr<VarDecl>> members;
    std::vector<std::shared_ptr<FuncDecl>> methods;
    ClassDecl(Token className, Token baseClassName, std::vector<std::shared_ptr<VarDecl>> classMembers, std::vector<std::shared_ptr<FuncDecl>> classMethods)
        : name(std::move(className)), baseClass(std::move(baseClassName)), members(std::move(classMembers)), methods(std::move(classMethods)) {}
};

class Program : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> declarations;
    Program(std::vector<std::shared_ptr<ASTNode>> decls)
        : declarations(std::move(decls)) {}
};

#endif
