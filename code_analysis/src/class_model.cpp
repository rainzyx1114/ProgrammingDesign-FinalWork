#include "class_model.h"

MethodDef::MethodDef(const std::string& n, std::shared_ptr<FuncDecl> decl,
                     const std::string& cls, bool virt)
    : name(n), declaration(decl), definingClass(cls), isVirtual(virt) {
}

ClassDef::ClassDef(const std::string& n, const std::string& base)
    : name(n), baseClass(base) {
}

void ClassDef::addMember(const std::string& name, std::shared_ptr<Type> type) {
    // Implementation
}

void ClassDef::addMethod(const std::string& name, std::shared_ptr<FuncDecl> decl, bool isVirtual) {
    // Implementation
}

bool ClassDef::hasMember(const std::string& name) const {
    // Implementation
    return false;
}

bool ClassDef::hasMethod(const std::string& name) const {
    // Implementation
    return false;
}

std::shared_ptr<Type> ClassDef::getMemberType(const std::string& name) const {
    // Implementation
    return nullptr;
}

ClassModel::ClassModel() {
}

void ClassModel::defineClass(const std::string& name, const std::string& baseClass) {
    // Implementation
}

void ClassModel::addClass(const ClassDef& classDef) {
    // Implementation
}

ClassDef* ClassModel::getClass(const std::string& name) {
    // Implementation
    return nullptr;
}

const ClassDef* ClassModel::getClass(const std::string& name) const {
    // Implementation
    return nullptr;
}

bool ClassModel::isDefined(const std::string& name) const {
    // Implementation
    return false;
}

std::string ClassModel::getBaseClass(const std::string& className) const {
    // Implementation
    return "";
}

bool ClassModel::isSubclassOf(const std::string& derived, const std::string& base) const {
    // Implementation
    return false;
}

std::string ClassModel::resolveVirtualMethod(const std::string& className, const std::string& methodName) const {
    // Implementation
    return "";
}

std::shared_ptr<Object> ClassModel::createInstance(const std::string& className) {
    // Implementation
    return std::make_shared<Object>(className);
}
