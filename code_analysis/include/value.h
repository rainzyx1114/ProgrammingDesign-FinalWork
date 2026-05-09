#ifndef VALUE_H
#define VALUE_H

#include <string>
#include <memory>
#include <map>

class Object;

class Value {
public:
    enum Type {
        UNINITIALIZED,
        INT,
        FLOAT,
        BOOL,
        OBJECT_REF,
        POINTER,
        ARRAY
    };
    
    Type type;
    int intVal;
    float floatVal;
    bool boolVal;
    std::shared_ptr<Object> objectRef;
    std::shared_ptr<void> pointerVal;
    
    Value();
    explicit Value(int i);
    explicit Value(float f);
    explicit Value(bool b);
    explicit Value(std::shared_ptr<Object> obj);
    
    std::string toString() const;
    int toInt() const;
    float toFloat() const;
    bool toBool() const;
};

class Object {
public:
    std::string className;
    std::map<std::string, Value> members;
    
    Object(const std::string& name) : className(name) {}
    
    Value getMember(const std::string& name) const;
    void setMember(const std::string& name, const Value& val);
};

#endif
