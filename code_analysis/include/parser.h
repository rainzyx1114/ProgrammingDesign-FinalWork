#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <vector>
#include <string>
#include "lexer.h"
#include "ast.h"

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    bool hadError;
    
public:
    explicit Parser(const std::vector<Token>& toks);
    
    std::shared_ptr<Program> parse();
    
private:
    // Helper methods
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    bool match(const TokenType& type);
    Token consume(TokenType type, const std::string& message);
    Token consume(const std::vector<TokenType>& types, const std::string& message);
    bool isAtEnd() const;
    std::shared_ptr<Expr> finishCall(std::shared_ptr<Expr> callee);
    Token error(const std::string& message);
    
    // Parsing methods
    std::shared_ptr<ASTNode> declaration();
    std::shared_ptr<ClassDecl> classDeclaration();
    std::shared_ptr<FuncDecl> functionDeclaration(std::shared_ptr<Type> returnType, Token name, bool isVirtual = false);
    std::shared_ptr<VarDecl> variableDeclaration(std::shared_ptr<Type> type, Token name);
    
    std::shared_ptr<Stmt> statement();
    std::shared_ptr<Block> block();
    std::shared_ptr<IfStmt> ifStatement();
    std::shared_ptr<WhileStmt> whileStatement();
    std::shared_ptr<ForStmt> forStatement();
    std::shared_ptr<ReturnStmt> returnStatement();
    std::shared_ptr<Stmt> expressionStatement();
    
    std::shared_ptr<Expr> expression();
    std::shared_ptr<Expr> assignment();
    std::shared_ptr<Expr> logicalOr();
    std::shared_ptr<Expr> logicalAnd();
    std::shared_ptr<Expr> equality();
    std::shared_ptr<Expr> comparison();
    std::shared_ptr<Expr> term();
    std::shared_ptr<Expr> factor();
    std::shared_ptr<Expr> unary();
    std::shared_ptr<Expr> call();
    std::shared_ptr<Expr> primary();
};

#endif
