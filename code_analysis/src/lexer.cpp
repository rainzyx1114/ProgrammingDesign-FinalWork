#include "lexer.h"

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"class", TokenType::CLASS},
    {"else", TokenType::ELSE},
    {"false", TokenType::FALSE},
    {"for", TokenType::FOR},
    {"if", TokenType::IF},
    {"NULL", TokenType::NIL},
    {"print", TokenType::PRINT},
    {"return", TokenType::RETURN},
    {"public", TokenType::PUBLIC},
    {"this", TokenType::THIS},
    {"true", TokenType::TRUE},
    {"virtual", TokenType::VIRTUAL},
    {"int", TokenType::INT},
    {"double", TokenType::DOUBLE},
    {"bool", TokenType::BOOL},
    {"float", TokenType::FLOAT},
    {"char", TokenType::CHAR},
    {"void", TokenType::VOID},
    {"string", TokenType::STRING_TYPE},
    {"struct", TokenType::STRUCT},
    {"const", TokenType::CONST},
    {"while", TokenType::WHILE}
};

Token::Token(TokenType t, const std::string& lex, int line, int col)
    : type(t), lexeme(lex), lineNumber(line), columnNumber(col) {
}

Lexer::Lexer(const std::string& src): source(src), current(0), start(0), lineNumber(1), columnNumber(1), tokens() {
}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start = current;
        Token token = nextToken();
        if (token.type == TokenType::UNKNOWN) {
            if (token.lexeme.empty()) {
                continue;
            }
            tokens.push_back(token);
            break;
        }
        tokens.push_back(token);
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
        case ':': token = makeToken(TokenType::COLON); break;
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
        case '&':
            token = makeToken(match('&') ? TokenType::AND : TokenType::UNKNOWN);
            break;
        case '|':
            token = makeToken(match('|') ? TokenType::OR : TokenType::UNKNOWN);
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
            columnNumber = 0;
            break;
        case '"':
            token = readString();
            break;
        default:
            if (std::isdigit(c)) {
                token = readNumber();
            } else if (std::isalpha(c) || c == '_') {
                token = readIdentifier();
            } else {
                token = makeToken(TokenType::UNKNOWN);
            }
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
    return Token(type, lexeme, lineNumber, columnNumber - 1);
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

char Lexer::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

Token Lexer::readString() {
    while( peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            lineNumber++;
            columnNumber = 0;
        }
        advance();
    }
    if (!isAtEnd()) {
        advance(); 
    }
    std::string value = source.substr(start + 1, current - start - 2);
    return Token(TokenType::STRING, value, lineNumber, columnNumber - 1);
}

Token Lexer::readNumber() {
    while(std::isdigit(peek())) {advance();}
    if (peek() == '.' && std::isdigit(peekNext())) {
        advance();
        while(std::isdigit(peek())) {advance();}
    }
    std::string value = source.substr(start, current - start);
    return Token(TokenType::NUMBER, value, lineNumber, columnNumber - 1);
}

Token Lexer::readIdentifier() {
    while(std::isalnum(peek()) || peek() == '_') {advance();}
    std::string value = source.substr(start, current - start);
    auto it = keywords.find(value);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
    return Token(type, value, lineNumber, columnNumber - 1);
}

std::string Token::toString() const {
    return lexeme;
}