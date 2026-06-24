#include "types.h"

std::string PrimitiveType::toString() const {
    switch (kind) {
        case INT: return "int";
        case DOUBLE: return "double";
        case FLOAT: return "float";
        case BOOL: return "bool";
        case CHAR: return "char";
        case VOID: return "void";
        case STRING_TYPE: return "string";
        case STRUCT: return "struct";
        case CONST: return "const";
        default: return "unknown";
    }
}

bool PrimitiveType::equals(const Type* other) const {
    auto* p = dynamic_cast<const PrimitiveType*>(other);
    if (!p) return false;
    return kind == p->kind;
}

std::string PointerType::toString() const {
    if (pointee) {
        return pointee->toString() + "*";
    }
    return "void*";
}

bool PointerType::equals(const Type* other) const {
    auto* p = dynamic_cast<const PointerType*>(other);
    if (!p) return false;
    if (!pointee && !p->pointee) return true;
    if (!pointee || !p->pointee) return false;
    return pointee->equals(p->pointee.get());
}

std::string ArrayType::toString() const {
    if (elementType) {
        return elementType->toString() + "[" + std::to_string(size) + "]";
    }
    return "unknown[]";
}

bool ArrayType::equals(const Type* other) const {
    auto* a = dynamic_cast<const ArrayType*>(other);
    if (!a) return false;
    if (size != a->size) return false;
    if (!elementType && !a->elementType) return true;
    if (!elementType || !a->elementType) return false;
    return elementType->equals(a->elementType.get());
}

std::string ClassType::toString() const {
    return className;
}

bool ClassType::equals(const Type* other) const {
    auto* c = dynamic_cast<const ClassType*>(other);
    if (!c) return false;
    return className == c->className;
}

std::string ReferenceType::toString() const {
    if (refType) {
        return refType->toString() + "&";
    }
    return "void&";
}

bool ReferenceType::equals(const Type* other) const {
    auto* r = dynamic_cast<const ReferenceType*>(other);
    if (!r) return false;
    if (!refType && !r->refType) return true;
    if (!refType || !r->refType) return false;
    return refType->equals(r->refType.get());
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
