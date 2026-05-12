#include "parser.h"

Parser::Parser(const std::vector<Token>& toks):tokens(toks), current(0) {}

std::shared_ptr<Program> Parser::parse() {
    std::vector<std::shared_ptr<Decl>> statements;
    while (!isAtEnd()) {
        auto decl = declaration();
        if (decl) {
            statements.push_back(decl);
        }
    }
    return std::make_shared<Program>(std::move(statements));
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

bool Parser::match(const TokenType& type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {return advance();}
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
    if (match({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL, TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE, TokenType::STRUCT, TokenType::CONST})) {
        return variableDeclaration();
    }
    return statement();
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
    Token typeToken = previous();
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    Expr initializer = nullptr;
    if (match(TokenType::EQUAL)) {
        initializer = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<VarDecl>(name, Type::createType(typeToken.type), initializer);
}

std::shared_ptr<Stmt> Parser::statement() {
    if (match(TokenType::LEFT_BRACE)) {
        return block();
    }
    if (match(TokenType::IF)) {
        return ifStatement();
    }
    if (match(TokenType::WHILE)) {
        return whileStatement();
    }
    return expressionStatement();
}

std::shared_ptr<Block> Parser::block() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        auto stmt = statement();
        if (stmt) {
            statements.push_back(stmt);
        }
    }
    return std::make_shared<Block>(std::move(statements));
}

std::shared_ptr<IfStmt> Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
    auto thenBranch = statement();
    std::shared_ptr<Stmt> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = statement();
    }
    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
}

std::shared_ptr<WhileStmt> Parser::whileStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
    auto body = statement();
    return std::make_shared<WhileStmt>(condition, body);
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
    auto expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<Exprstmt>(expr);
}

std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expr> Parser::assignment() {
    auto expr = LogicalOr();
    if (match(TokenType::EQUAL)) {
        Token equals = previous();
        auto value = assignment();
        if (std::dynamic_pointer_cast<Variable>(expr)) {
            Token name = std::dynamic_pointer_cast<Variable>(expr)->name;
            return std::make_shared<Assign>(name, value);
        }
    }
    return expr;
}

std::shared_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match(TokenType::OR)) {
        Token op = previous();
        auto right = LogicalAnd();
        expr = std::make_shared<LogicalOp>(expr, op, right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();
    while (match(TokenType::AND)) {
        Token op = previous();
        auto right = equality();
        expr = std::make_shared<LogicalOp>(expr, op, right);
    }
    return expr;
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
    if (match({TokenType::NUMBER, TokenType::STRING})) {
        return std::make_shared<Literal>(previous());
    }
    if (match({TokenType::TRUE, TokenType::FALSE, TokenType::NIL})) {
        return std::make_shared<Literal>(previous());
    }
    if (match(TokenType::LEFT_PAREN)) {
        auto expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }
    if (match(TokenType::IDENTIFIER)) {
        return std::make_shared<Variable>(previous());
    }
    return nullptr;
}
