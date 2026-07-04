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

Value Value::pointerFromId(int heapObjId) {
    Value v;
    v.type = POINTER;
    v.intVal = heapObjId;
    return v;
}

int Value::getPointerId() const {
    return intVal;
}

std::string Value::toString() const {
    switch (type) {
        case UNINITIALIZED: return "uninitialized";
        case INT: return std::to_string(intVal);
        case FLOAT: return std::to_string(floatVal);
        case BOOL: return boolVal ? "true" : "false";
        case OBJECT_REF: {
            if (!objectRef) return "object(null)";
            if (objectRef->className == "array") {
                std::string result = "[";
                bool first = true;
                for (int i = 0; ; i++) {
                    auto it = objectRef->members.find(std::to_string(i));
                    if (it == objectRef->members.end()) break;
                    if (!first) result += ", ";
                    first = false;
                    result += it->second.toString();
                }
                result += "]";
                return result;
            }
            return "object(" + objectRef->className + ")";
        }
        case POINTER: return "ptr->obj" + std::to_string(intVal);
        default: return "";
    }
}

int Value::toInt() const {
    return intVal;
}

float Value::toFloat() const {
    return floatVal;
}

bool Value::toBool() const {
    return boolVal;
}

Value Object::getMember(const std::string& name) const {
    auto it = members.find(name);
    if (it != members.end()) return it->second;
    return Value();
}

void Object::setMember(const std::string& name, const Value& val) {
    members[name] = val;
}
