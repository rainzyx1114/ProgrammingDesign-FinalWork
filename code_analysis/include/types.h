#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <memory>
#include <vector>

class Type {
public:
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type* other) const = 0;
};

class PrimitiveType : public Type {
public:
    enum Kind {
        INT,
        FLOAT,
        BOOL,
        VOID
    };
    
    Kind kind;
    explicit PrimitiveType(Kind k) : kind(k) {}
    
    std::string toString() const override;
    bool equals(const Type* other) const override;
};

class PointerType : public Type {
public:
    std::shared_ptr<Type> pointee;
    
    explicit PointerType(std::shared_ptr<Type> t) : pointee(t) {}
    
    std::string toString() const override;
    bool equals(const Type* other) const override;
};

class ArrayType : public Type {
public:
    std::shared_ptr<Type> elementType;
    int size;
    
    ArrayType(std::shared_ptr<Type> t, int s) : elementType(t), size(s) {}
    
    std::string toString() const override;
    bool equals(const Type* other) const override;
};

class ClassType : public Type {
public:
    std::string className;
    
    explicit ClassType(const std::string& name) : className(name) {}
    
    std::string toString() const override;
    bool equals(const Type* other) const override;
};

class ReferenceType : public Type {
public:
    std::shared_ptr<Type> refType;
    
    explicit ReferenceType(std::shared_ptr<Type> t) : refType(t) {}
    
    std::string toString() const override;
    bool equals(const Type* other) const override;
};

#endif
