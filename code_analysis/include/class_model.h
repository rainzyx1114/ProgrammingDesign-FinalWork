#ifndef CLASS_MODEL_H
#define CLASS_MODEL_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "ast.h"
#include "types.h"
#include "value.h"

class MethodDef {
public:
    std::string name;
    std::shared_ptr<FuncDecl> declaration;
    std::string definingClass;  // Which class defined this method
    bool isVirtual;
    
    MethodDef(const std::string& n, std::shared_ptr<FuncDecl> decl, 
              const std::string& cls, bool virt = false);
};

class ClassDef {
public:
    std::string name;
    std::string baseClass;  // Parent class name
    std::vector<std::pair<std::string, std::shared_ptr<Type>>> members;  // (name, type)
    std::map<std::string, MethodDef> methods;  // method_name -> MethodDef
    std::map<std::string, std::string> vtable;  // virtual_method -> actual_class_method
    
    ClassDef(const std::string& n, const std::string& base = "");
    
    void addMember(const std::string& name, std::shared_ptr<Type> type);
    void addMethod(const std::string& name, std::shared_ptr<FuncDecl> decl, bool isVirtual = false);
    bool hasMember(const std::string& name) const;
    bool hasMethod(const std::string& name) const;
    std::shared_ptr<Type> getMemberType(const std::string& name) const;
};

class ClassModel {
private:
    std::map<std::string, ClassDef> classes;
    
public:
    ClassModel();
    
    // Class definition operations
    void defineClass(const std::string& name, const std::string& baseClass = "");
    void addClass(const ClassDef& classDef);
    ClassDef* getClass(const std::string& name);
    const ClassDef* getClass(const std::string& name) const;
    bool isDefined(const std::string& name) const;
    
    // Inheritance operations
    std::string getBaseClass(const std::string& className) const;
    bool isSubclassOf(const std::string& derived, const std::string& base) const;
    
    // Virtual method resolution
    std::string resolveVirtualMethod(const std::string& className, const std::string& methodName) const;
    
    // Object creation
    std::shared_ptr<Object> createInstance(const std::string& className);
};

#endif
