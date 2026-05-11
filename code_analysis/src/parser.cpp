#include "parser.h"

Parser::Parser(const std::vector<Token>& toks): tokens(toks), current(0) {
}

std::shared_ptr<Program> Parser::parse() {
    // Implementation
    return std::make_shared<Program>();
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) {current++;}
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (const auto& type: types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    // Implementation
    return Token(TokenType::UNKNOWN, "", 0, 0);
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::eof;
}

void Parser::error(const std::string& message) {
    // Implementation
}

void Parser::synchronize() {
    // Implementation
}

std::shared_ptr<Decl> Parser::declaration() {
    // Implementation
    return nullptr;
}

std::shared_ptr<ClassDecl> Parser::classDeclaration() {
    // Implementation
    return nullptr;
}

std::shared_ptr<FuncDecl> Parser::functionDeclaration() {
    // Implementation
    return nullptr;
}

std::shared_ptr<VarDecl> Parser::variableDeclaration() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Stmt> Parser::statement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Block> Parser::block() {
    // Implementation
    return nullptr;
}

std::shared_ptr<IfStmt> Parser::ifStatement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<WhileStmt> Parser::whileStatement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<ForStmt> Parser::forStatement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<ReturnStmt> Parser::returnStatement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Stmt> Parser::expressionStatement() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Expr> Parser::expression() {
    return equality();
}

std::shared_ptr<Expr> Parser::assignment() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Expr> Parser::logicalOr() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Expr> Parser::logicalAnd() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    while (match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::comparison() {
    auto expr = term();
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
        Token op = previous();
        auto right = term();
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::term() {
    auto expr = factor();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        auto right = factor();
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::factor() {
    auto expr = unary();
    while (match({TokenType::STAR, TokenType::SLASH})) {
        Token op = previous();
        auto right = unary();
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS})) {
        Token op = previous();
        auto right = unary();
        return std::make_shared<UnaryOp>(op, right);
    }
    return primary();
}

std::shared_ptr<Expr> Parser::postfix() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Expr> Parser::primary() {
    // Implementation
    return nullptr;
}

std::shared_ptr<Type> Parser::parseType() {
    // Implementation
    return nullptr;
}
