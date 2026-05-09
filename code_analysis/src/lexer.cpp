#include "lexer.h"

Token::Token(TokenType t, const std::string& lex, int line, int col)
    : type(t), lexeme(lex), lineNumber(line), columnNumber(col) {
}

std::string Token::toString() const {
    // Implementation
    return "";
}

Lexer::Lexer(const std::string& src): source(src), current(0), start(0), lineNumber(1), columnNumber(1), tokens() {
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start = current;
        Token token = nextToken();
        if (token.type != TokenType::UNKNOWN) {
            tokens.push_back(token);
        }
    }
    tokens.push_back(Token(TokenType::eof, "", lineNumber, columnNumber));
    return tokens;
}

Token Lexer::nextToken() {
    char c = advance();
    Token token(TokenType::UNKNOWN, "", lineNumber, columnNumber);
    switch (c) {
        case '(': token = makeToken(TokenType::LEFT_PAREN); break;
        case ')': token = makeToken(TokenType::RIGHT_PAREN); break;
        case '{': token = makeToken(TokenType::LEFT_BRACE); break;
        case '}': token = makeToken(TokenType::RIGHT_BRACE); break;
        case ',': token = makeToken(TokenType::COMMA); break;
        case '.': token = makeToken(TokenType::DOT); break;
        case '-': token = makeToken(TokenType::MINUS); break;
        case '+': token = makeToken(TokenType::PLUS); break;
        case ';': token = makeToken(TokenType::SEMICOLON); break;
        case '*': token = makeToken(TokenType::STAR); break;
        case '!':
            token = makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
            break;
        case '=':
            token = makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            break;
        case '<':
            token = makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;
        case '>':
            token = makeToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;
        case '/':
            if (match('/')) {
            while (peek() != '\n' && !isAtEnd()) advance();
            } else {
            token = makeToken(TokenType::SLASH);
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            lineNumber++;
            columnNumber = 1;
            break;
    }
    return token;
}

char Lexer::advance() {
    char temp = source[current];
    current ++;
    columnNumber ++;
    return temp;
}

bool Lexer::isAtEnd() const {
    return current >= source.length();
}

Token Lexer::makeToken(TokenType type) {
    std::string lexeme = source.substr(start, current - start);
    return Token(type, lexeme, lineNumber, columnNumber);
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    columnNumber++;
    return true;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}