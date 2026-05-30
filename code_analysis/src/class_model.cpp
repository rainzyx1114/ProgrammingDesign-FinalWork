#include "class_model.h"

MethodDef::MethodDef(const std::string& n, std::shared_ptr<FuncDecl> decl,
                     const std::string& cls, bool virt)
    : name(n), declaration(decl), definingClass(cls), isVirtual(virt) {
}

ClassDef::ClassDef(const std::string& n, const std::string& base)
    : name(n), baseClass(base) {
}

void ClassDef::addMember(const std::string& name, std::shared_ptr<Type> type) {
    members.push_back(std::make_pair(name, type));
}

void ClassDef::addMethod(const std::string& name, std::shared_ptr<FuncDecl> decl, bool isVirtual) {
    methods[name] = MethodDef(name, decl, this->name, isVirtual);
}

bool ClassDef::hasMember(const std::string& name) const {
    for (const auto& p: members) {
        if (name == p.first) {
            return true;
        }
    }
    return false;
}

bool ClassDef::hasMethod(const std::string& name) const {
    return methods.find(name) != methods.end();
}

std::shared_ptr<Type> ClassDef::getMemberType(const std::string& name) const {
    if (hasMember(name)) {
        for (const auto& p: members) {
            if (name == p.first) {
                return p.second;
            }
        }
    }
    return nullptr;
}

void ClassModel::defineClass(const std::string& name, const std::string& baseClass) {
    addClass(ClassDef(name, baseClass));
}

void ClassModel::addClass(const ClassDef& classDef) {
    classes[classDef.name] = classDef;
}

ClassDef* ClassModel::getClass(const std::string& name) {
    auto it = classes.find(name);
    if (it == classes.end()) return nullptr;
    return &it->second;
}

const ClassDef* ClassModel::getClass(const std::string& name) const {
    auto it = classes.find(name);
    if (it == classes.end()) return nullptr;
    return &it->second;
}

bool ClassModel::isDefined(const std::string& name) const {
    return classes.find(name) != classes.end();
}

std::string ClassModel::getBaseClass(const std::string& className) {
    auto it = classes.find(className);
    if (it == classes.end()) return "";
    return it->second.baseClass;
}

bool ClassModel::isSubclassOf(const std::string& derived, const std::string& base) {
    auto it = classes.find(derived);
    if (it == classes.end()) return false;
    std::string parent = it->second.baseClass;
    while (!parent.empty()) {
        if (parent == base) {
            return true;
        }
        auto parentIt = classes.find(parent);
        if (parentIt == classes.end()) break;
        parent = parentIt->second.baseClass;
    }
    return false;
}

std::string ClassModel::resolveVirtualMethod(const std::string& className, const std::string& methodName) const {
    const ClassDef* current = getClass(className);
    const ClassDef* lastVirtualOwner = nullptr;
    while (current) {
        auto it = current->methods.find(methodName);
        if (it != current->methods.end()) {
            if (it->second.isVirtual) {
                lastVirtualOwner = current;
            }
        }
        if (current->baseClass.empty()) break;
        current = getClass(current->baseClass);
    }
    if (lastVirtualOwner) {
        return lastVirtualOwner->name + "::" + methodName;
    }
    return "";
}

std::shared_ptr<Object> ClassModel::createInstance(const std::string& className) {
    return std::make_shared<Object>(className);
}
