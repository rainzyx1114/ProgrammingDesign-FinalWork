#ifndef CLASS_MODEL_H
#define CLASS_MODEL_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "ast.h"
#include "types.h"
#include "value.h"

inline std::string accessLevelToString(AccessLevel level) {
    switch (level) {
        case AccessLevel::PUBLIC_ACCESS: return "public";
        case AccessLevel::PRIVATE_ACCESS: return "private";
        case AccessLevel::PROTECTED_ACCESS: return "protected";
        default: return "private";
    }
}

struct MemberEntry {
    std::string name;
    std::shared_ptr<Type> type;
    AccessLevel accessLevel = AccessLevel::PRIVATE_ACCESS;
};

class MethodDef {
public:
    std::string name;
    std::shared_ptr<FuncDecl> declaration;
    std::string definingClass;  // Which class defined this method
    bool isVirtual;
    AccessLevel accessLevel = AccessLevel::PRIVATE_ACCESS;

    MethodDef() = default;
    MethodDef(const std::string& n, std::shared_ptr<FuncDecl> decl,
              const std::string& cls, bool virt = false);
};

class ClassDef {
public:
    std::string name;
    std::string baseClass;  // Parent class name
    std::vector<MemberEntry> members;  // (name, type, accessLevel)
    std::map<std::string, MethodDef> methods;  // method_name -> MethodDef
    std::map<std::string, std::string> vtable;  // virtual_method -> actual_class_method

    ClassDef() = default;
    ClassDef(const std::string& n, const std::string& base = "");

    void addMember(const std::string& name, std::shared_ptr<Type> type, AccessLevel access = AccessLevel::PRIVATE_ACCESS);
    void addMethod(const std::string& name, std::shared_ptr<FuncDecl> decl, bool isVirtual = false, AccessLevel access = AccessLevel::PRIVATE_ACCESS);
    bool hasMember(const std::string& name) const;
    bool hasMethod(const std::string& name) const;
    std::shared_ptr<Type> getMemberType(const std::string& name) const;
    AccessLevel getMemberAccess(const std::string& name) const;
    AccessLevel getMethodAccess(const std::string& name) const;
};

class ClassModel {
public:
    std::map<std::string, ClassDef> classes;
    
    ClassModel()=default;
    
    // Class definition operations
    void defineClass(const std::string& name, const std::string& baseClass = "");
    void addClass(const ClassDef& classDef);
    ClassDef* getClass(const std::string& name);
    const ClassDef* getClass(const std::string& name) const;
    bool isDefined(const std::string& name) const;
    
    // Inheritance operations
    std::string getBaseClass(const std::string& className);
    bool isSubclassOf(const std::string& derived, const std::string& base);
    
    // Virtual method resolution
    std::string resolveVirtualMethod(const std::string& className, const std::string& methodName) const;
    
    // Object creation
    std::shared_ptr<Object> createInstance(const std::string& className);
};

#endif
