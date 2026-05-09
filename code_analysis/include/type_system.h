#ifndef TYPE_SYSTEM_H
#define TYPE_SYSTEM_H

#include <string>
#include <memory>
#include <map>
#include "types.h"

class TypeSystem {
private:
    std::map<std::string, std::shared_ptr<ClassType>> classTypes;
    std::map<std::string, std::string> inheritanceMap;  // class -> baseClass
    
public:
    TypeSystem();
    
    // Type operations
    bool isAssignableFrom(const Type* target, const Type* source);
    bool canImplicitConvert(const Type* from, const Type* to);
    
    // Class operations
    void registerClass(const std::string& name, const std::string& baseClass = "");
    bool isClassDefined(const std::string& name) const;
    std::string getBaseClass(const std::string& className) const;
    bool isSubclassOf(const std::string& derived, const std::string& base) const;
    
    // Primitive type factories
    std::shared_ptr<PrimitiveType> getIntType();
    std::shared_ptr<PrimitiveType> getFloatType();
    std::shared_ptr<PrimitiveType> getBoolType();
    std::shared_ptr<PrimitiveType> getVoidType();
};

#endif
