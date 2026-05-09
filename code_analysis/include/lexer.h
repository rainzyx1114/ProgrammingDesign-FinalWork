#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <memory>


enum class TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE, UNKNOWN,
    eof
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    int lineNumber;
    int columnNumber;
    
    Token(TokenType t, const std::string& lex, int line, int col);
    std::string toString() const;
};

class Lexer {
private:
    std::string source;
    size_t current;
    size_t start;
    int lineNumber;
    int columnNumber;
    std::vector<Token> tokens;

public:
    explicit Lexer(const std::string& src);
    
    std::vector<Token> tokenize();
    
private:
    Token nextToken();
    char advance();
    bool isAtEnd() const;
    Token makeToken(TokenType type);
    bool match(char expected);
    char peek() const;
};

#endif
