#include "value.h"

Value::Value()
    : type(UNINITIALIZED), intVal(0), floatVal(0.0f), boolVal(false) {
}

Value::Value(int i)
    : type(INT), intVal(i), floatVal(0.0f), boolVal(false) {
}

Value::Value(float f)
    : type(FLOAT), intVal(0), floatVal(f), boolVal(false) {
}

Value::Value(bool b)
    : type(BOOL), intVal(0), floatVal(0.0f), boolVal(b) {
}

Value::Value(std::shared_ptr<Object> obj)
    : type(OBJECT_REF), intVal(0), floatVal(0.0f), boolVal(false), objectRef(obj) {
}

std::string Value::toString() const {
    // Implementation
    return "";
}

int Value::toInt() const {
    // Implementation
    return 0;
}

float Value::toFloat() const {
    // Implementation
    return 0.0f;
}

bool Value::toBool() const {
    // Implementation
    return false;
}

Value Object::getMember(const std::string& name) const {
    // Implementation
    return Value();
}

void Object::setMember(const std::string& name, const Value& val) {
    // Implementation
}
