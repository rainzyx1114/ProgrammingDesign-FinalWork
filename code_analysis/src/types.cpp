#include "types.h"

std::string PrimitiveType::toString() const {
    // Implementation
    return "";
}

bool PrimitiveType::equals(const Type* other) const {
    // Implementation
    return false;
}

std::string PointerType::toString() const {
    // Implementation
    return "";
}

bool PointerType::equals(const Type* other) const {
    // Implementation
    return false;
}

std::string ArrayType::toString() const {
    // Implementation
    return "";
}

bool ArrayType::equals(const Type* other) const {
    // Implementation
    return false;
}

std::string ClassType::toString() const {
    // Implementation
    return "";
}

bool ClassType::equals(const Type* other) const {
    // Implementation
    return false;
}

std::string ReferenceType::toString() const {
    // Implementation
    return "";
}

bool ReferenceType::equals(const Type* other) const {
    // Implementation
    return false;
}

std::shared_ptr<Type> Type::createType(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::INT: return std::make_shared<PrimitiveType>(PrimitiveType::INT);
        case TokenType::DOUBLE: return std::make_shared<PrimitiveType>(PrimitiveType::DOUBLE);
        case TokenType::FLOAT: return std::make_shared<PrimitiveType>(PrimitiveType::FLOAT);
        case TokenType::BOOL: return std::make_shared<PrimitiveType>(PrimitiveType::BOOL);
        case TokenType::CHAR: return std::make_shared<PrimitiveType>(PrimitiveType::CHAR);
        case TokenType::VOID: return std::make_shared<PrimitiveType>(PrimitiveType::VOID);
        case TokenType::STRING_TYPE: return std::make_shared<PrimitiveType>(PrimitiveType::STRING_TYPE);
        case TokenType::STRUCT: return std::make_shared<PrimitiveType>(PrimitiveType::STRUCT);
        case TokenType::CONST: return std::make_shared<PrimitiveType>(PrimitiveType::CONST);
        default: return nullptr;  
    }
}