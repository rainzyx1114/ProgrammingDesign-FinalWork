#include "type_system.h"

TypeSystem::TypeSystem() {
}

bool TypeSystem::isAssignableFrom(const Type* target, const Type* source) {
    // Implementation
    return false;
}

bool TypeSystem::canImplicitConvert(const Type* from, const Type* to) {
    // Implementation
    return false;
}

void TypeSystem::registerClass(const std::string& name, const std::string& baseClass) {
    // Implementation
}

bool TypeSystem::isClassDefined(const std::string& name) const {
    // Implementation
    return false;
}

std::string TypeSystem::getBaseClass(const std::string& className) const {
    // Implementation
    return "";
}

bool TypeSystem::isSubclassOf(const std::string& derived, const std::string& base) const {
    // Implementation
    return false;
}

std::shared_ptr<PrimitiveType> TypeSystem::getIntType() {
    // Implementation
    return std::make_shared<PrimitiveType>(PrimitiveType::INT);
}

std::shared_ptr<PrimitiveType> TypeSystem::getFloatType() {
    // Implementation
    return std::make_shared<PrimitiveType>(PrimitiveType::FLOAT);
}

std::shared_ptr<PrimitiveType> TypeSystem::getBoolType() {
    // Implementation
    return std::make_shared<PrimitiveType>(PrimitiveType::BOOL);
}

std::shared_ptr<PrimitiveType> TypeSystem::getVoidType() {
    // Implementation
    return std::make_shared<PrimitiveType>(PrimitiveType::VOID);
}
