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

    // Check if this is a type-led declaration (variable or function)
    // Primitive types always indicate a declaration
    if (match({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL,
                TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE,
                TokenType::STRUCT, TokenType::CONST})) {
        // Build the base type from the consumed token
        Token typeToken = previous();
        std::shared_ptr<Type> type = Type::createType(typeToken.type);
        // Pointer modifiers (before variable name): int*, int**
        while (match(TokenType::STAR)) {
            type = std::make_shared<PointerType>(type);
        }

        // Consume the variable/function name
        Token name = consume(TokenType::IDENTIFIER, "Expect variable or function name.");

        // Array modifiers (after variable name): int arr[5], int arr[5][10]
        while (match(TokenType::LEFT_BRACKET)) {
            int size = 0;
            if (!check(TokenType::RIGHT_BRACKET)) {
                Token sizeToken = consume(TokenType::NUMBER, "Expect array size.");
                size = std::stoi(sizeToken.lexeme);
            }
            consume(TokenType::RIGHT_BRACKET, "Expect ']' after array size.");
            type = std::make_shared<ArrayType>(type, size);
        }

        if (match(TokenType::LEFT_PAREN)) {
            return functionDeclaration(type, name, isVirtual);
        } else {
            return variableDeclaration(type, name);
        }
    }

    // Class-type declaration: "ClassName varName" or "ClassName* varName" etc.
    // Use peek-ahead to distinguish from function calls: "foo(bar);"
    if (check(TokenType::IDENTIFIER)) {
        size_t savedPos = current;
        advance(); // eat the first identifier
        // If next is IDENTIFIER or STAR → it's a type declaration (ClassName varName or ClassName* varName)
        // LEFT_BRACKET after IDENTIFIER is an array access, not a type
        if (check(TokenType::IDENTIFIER) || check(TokenType::STAR)) {
            // Restore position and parse as type + name
            current = savedPos;
            Token typeToken = advance(); // consume the class name
            std::shared_ptr<Type> type = std::make_shared<ClassType>(typeToken.lexeme);

            // Pointer modifiers (before variable name)
            while (match(TokenType::STAR)) {
                type = std::make_shared<PointerType>(type);
            }

            // Consume variable name
            Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

            // Array modifiers (after variable name): ClassName arr[5]
            while (match(TokenType::LEFT_BRACKET)) {
                int size = 0;
                if (!check(TokenType::RIGHT_BRACKET)) {
                    Token sizeToken = consume(TokenType::NUMBER, "Expect array size.");
                    size = std::stoi(sizeToken.lexeme);
                }
                consume(TokenType::RIGHT_BRACKET, "Expect ']' after array size.");
                type = std::make_shared<ArrayType>(type, size);
            }

            if (match(TokenType::LEFT_PAREN)) {
                return functionDeclaration(type, name, isVirtual);
            } else {
                return variableDeclaration(type, name);
            }
        }
        // Not a type declaration — restore and fall through to statement()
        current = savedPos;
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
    AccessLevel currentAccess = AccessLevel::PRIVATE_ACCESS;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        // Check for access specifier labels
        if (match(TokenType::PUBLIC) && match(TokenType::COLON)) {
            currentAccess = AccessLevel::PUBLIC_ACCESS;
            continue;
        }
        if (match(TokenType::PRIVATE) && match(TokenType::COLON)) {
            currentAccess = AccessLevel::PRIVATE_ACCESS;
            continue;
        }
        if (match(TokenType::PROTECTED) && match(TokenType::COLON)) {
            currentAccess = AccessLevel::PROTECTED_ACCESS;
            continue;
        }
        auto decl = declaration();
        if (decl) {
            if (auto varDecl = std::dynamic_pointer_cast<VarDecl>(decl)) {
                varDecl->accessLevel = currentAccess;
                members.push_back(varDecl);
            } else if (auto funcDecl = std::dynamic_pointer_cast<FuncDecl>(decl)) {
                funcDecl->accessLevel = currentAccess;
                methods.push_back(funcDecl);
            }
        }
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    return std::make_shared<ClassDecl>(name, superclass, std::move(members), std::move(methods));
}

std::shared_ptr<FuncDecl> Parser::functionDeclaration(std::shared_ptr<Type> returnType, Token name, bool isVirtual) {
    std::vector<std::pair<Token, std::shared_ptr<Type>>> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            Token paramTypeToken = consume({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT,
                                             TokenType::BOOL, TokenType::CHAR, TokenType::VOID,
                                             TokenType::STRING_TYPE, TokenType::STRUCT, TokenType::CONST,
                                             TokenType::IDENTIFIER},
                                            "Expect parameter type.");
            std::shared_ptr<Type> paramType;
            if (paramTypeToken.type == TokenType::IDENTIFIER) {
                paramType = std::make_shared<ClassType>(paramTypeToken.lexeme);
            } else {
                paramType = Type::createType(paramTypeToken.type);
            }
            // Pointer modifiers (before parameter name)
            while (match(TokenType::STAR)) {
                paramType = std::make_shared<PointerType>(paramType);
            }
            Token paramName = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            // Array modifiers (after parameter name): int arr[]
            while (match(TokenType::LEFT_BRACKET)) {
                int size = 0;
                if (!check(TokenType::RIGHT_BRACKET)) {
                    Token sizeToken = consume(TokenType::NUMBER, "Expect array size.");
                    size = std::stoi(sizeToken.lexeme);
                }
                consume(TokenType::RIGHT_BRACKET, "Expect ']' after array size.");
                paramType = std::make_shared<ArrayType>(paramType, size);
            }
            params.emplace_back(paramName, paramType);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");

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

std::shared_ptr<VarDecl> Parser::variableDeclaration(std::shared_ptr<Type> type, Token name) {
    std::shared_ptr<Expr> initializer = nullptr;
    if (match(TokenType::EQUAL)) {
        initializer = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<VarDecl>(name, type, initializer);
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
        auto decl = declaration();
        if (decl) {
            if (auto stmt = std::dynamic_pointer_cast<Stmt>(decl)) {
                statements.push_back(stmt);
            }
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
    } else if (match({TokenType::INT, TokenType::DOUBLE, TokenType::FLOAT, TokenType::BOOL,
                       TokenType::CHAR, TokenType::VOID, TokenType::STRING_TYPE,
                       TokenType::STRUCT, TokenType::CONST, TokenType::IDENTIFIER})) {
        Token typeToken = previous();
        std::shared_ptr<Type> type;
        if (typeToken.type == TokenType::IDENTIFIER) {
            type = std::make_shared<ClassType>(typeToken.lexeme);
        } else {
            type = Type::createType(typeToken.type);
        }
        while (match(TokenType::STAR)) {
            type = std::make_shared<PointerType>(type);
        }
        Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
        initializer = variableDeclaration(type, name);
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
        auto value = assignment();
        if (auto var = std::dynamic_pointer_cast<Variable>(expr)) {
            return std::make_shared<Assignment>(var->name, value);
        } else if (std::dynamic_pointer_cast<MemberAccess>(expr) ||
                   std::dynamic_pointer_cast<ArrayAccess>(expr)) {
            // l-value is a member or array access
            auto assign = std::make_shared<Assignment>(Token(TokenType::UNKNOWN, "", 0, 0), value);
            assign->target = expr;
            return assign;
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
    if (match({TokenType::BANG, TokenType::MINUS, TokenType::AMPERSAND, TokenType::STAR})) {
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
        } else if (match(TokenType::DOT)) {
            Token memberName = consume(TokenType::IDENTIFIER, "Expect member name after '.'.");
            expr = std::make_shared<MemberAccess>(expr, memberName.lexeme, false);
        } else if (match(TokenType::ARROW)) {
            Token memberName = consume(TokenType::IDENTIFIER, "Expect member name after '->'.");
            expr = std::make_shared<MemberAccess>(expr, memberName.lexeme, true);
        } else if (match(TokenType::LEFT_BRACKET)) {
            auto index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
            expr = std::make_shared<ArrayAccess>(expr, index);
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
    if (match(TokenType::NEW)) {
        Token className = consume(TokenType::IDENTIFIER, "Expect class name after 'new'.");
        std::vector<std::shared_ptr<Expr>> args;
        if (match(TokenType::LEFT_PAREN)) {
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')' after constructor arguments.");
        }
        return std::make_shared<NewExpr>(className.lexeme, std::move(args));
    }
    if (match(TokenType::IDENTIFIER)) {
        return std::make_shared<Variable>(previous());
    }
    return nullptr;
}
