#include "parser.h"
#include "types.h"

Parser::Parser(const std::vector<Token>& toks):tokens(toks), current(0), hadError(false) {}

std::shared_ptr<Program> Parser::parse() {
    std::vector<std::shared_ptr<ASTNode>> statements;
    while (!isAtEnd()) {
        auto decl = declaration();
        if (hadError || !decl) {
            return nullptr;
        }
        statements.push_back(decl);
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
    return error(message);
}

Token Parser::consume(const std::vector<TokenType>& types, const std::string& message) {
    for (auto type : types) {
        if (check(type)) {
            return advance();
        }
    }
    return error(message);
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::eof;
}

Token Parser::error(const std::string& message) {
    hadError = true;
    // Stop parsing immediately by skipping to end of token stream.
    current = tokens.size() > 0 ? tokens.size() - 1 : 0;
    return Token(TokenType::UNKNOWN, "", peek().lineNumber, peek().columnNumber);
}

std::shared_ptr<Expr> Parser::finishCall(std::shared_ptr<Expr> callee) {
    std::vector<std::shared_ptr<Expr>> args;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return std::make_shared<FunctionCall>(callee, std::move(args));
}

std::shared_ptr<ASTNode> Parser::declaration() {
    bool isVirtual = false;
    if (match(TokenType::VIRTUAL)) {
        isVirtual = true;
    }
    if (match({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL, TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE, TokenType::STRUCT, TokenType::CONST})) {
        Token typeToken = previous();
        Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
        if (match(TokenType::LEFT_PAREN)) {
            return functionDeclaration(typeToken, name, isVirtual);
        } else {
            return variableDeclaration(typeToken, name);
        }
    }
    if (match(TokenType::CLASS)) {
        return classDeclaration();
    }
    return statement();
}

std::shared_ptr<ClassDecl> Parser::classDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
    Token superclass = Token(TokenType::UNKNOWN, "", 0, 0);
    if (match(TokenType::COLON)) {
        consume(TokenType::PUBLIC, "Expect 'public' before superclass name.");
        superclass = consume(TokenType::IDENTIFIER, "Expect superclass name.");
    }
    if (!match(TokenType::LEFT_BRACE)) {
        error("Expect '{' before class body.");
        return nullptr;
    }
    std::vector<std::shared_ptr<VarDecl>> members;
    std::vector<std::shared_ptr<FuncDecl>> methods;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        auto decl = declaration();
        if (decl) {
            if (auto varDecl = std::dynamic_pointer_cast<VarDecl>(decl)) {
                members.push_back(varDecl);
            } else if (auto funcDecl = std::dynamic_pointer_cast<FuncDecl>(decl)) {
                methods.push_back(funcDecl);
            }
        }
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    return std::make_shared<ClassDecl>(name, superclass, std::move(members), std::move(methods));
}

std::shared_ptr<FuncDecl> Parser::functionDeclaration(Token returnTypeToken, Token name, bool isVirtual) {
    std::vector<std::pair<Token, std::shared_ptr<Type>>> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token paramTypeToken = consume({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL, TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE, TokenType::STRUCT, TokenType::CONST}, "Expect parameter type.");
            Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            params.emplace_back(paramName, Type::createType(paramTypeToken.type));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    std::shared_ptr<Type> returnType = Type::createType(returnTypeToken.type);

    std::shared_ptr<Block> body = nullptr;
    if (match(TokenType::LEFT_BRACE)) {
        body = block();
    } else {
        error("Expect '{' before function body.");
    }

    auto funcDecl = std::make_shared<FuncDecl>(name, std::move(params), returnType, body);
    funcDecl->isVirtual = isVirtual;
    return funcDecl;
}

std::shared_ptr<VarDecl> Parser::variableDeclaration(Token typeToken, Token name) {
    std::shared_ptr<Expr> initializer = nullptr;
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
    if (match(TokenType::FOR)) {
        return forStatement();
    }
    if (match(TokenType::RETURN)) {
        return returnStatement();
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
    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
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
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    std::shared_ptr<ASTNode> initializer;
    if (match(TokenType::SEMICOLON)) {
        initializer = nullptr;
    } else if (match({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL, TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE, TokenType::STRUCT, TokenType::CONST})) {
        Token typeToken = previous();
        Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
        initializer = variableDeclaration(typeToken, name);
    } else {
        initializer = expressionStatement();
    }
    std::shared_ptr<Expr> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");
    std::shared_ptr<Expr> update = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        update = expression();
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before for loop body.");
    auto body = block();
    return std::make_shared<ForStmt>(initializer, condition, update, body);
}

std::shared_ptr<ReturnStmt> Parser::returnStatement() {
    std::shared_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    consume(TokenType::SEMICOLON, "Expect a ';' afer return value.");
    return std::make_shared<ReturnStmt>(value);
}

std::shared_ptr<Stmt> Parser::expressionStatement() {
    auto expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<ExprStmt>(expr);
}

std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expr> Parser::assignment() {
    auto expr = logicalOr();
    if (match(TokenType::EQUAL)) {
        Token equals = previous();
        auto value = assignment();
        if (std::dynamic_pointer_cast<Variable>(expr)) {
            Token name = std::dynamic_pointer_cast<Variable>(expr)->name;
            return std::make_shared<Assignment>(name, value);
        }
    }
    return expr;
}

std::shared_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match(TokenType::OR)) {
        Token op = previous();
        auto right = logicalAnd();
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
    return call();
}

std::shared_ptr<Expr> Parser::call() {
    auto expr = primary();
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall(expr);
        } else {
            break;
        }
    }
    return expr;
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
