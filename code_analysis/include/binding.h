#ifndef BINDING_H
#define BINDING_H

#include <map>
#include <memory>
#include <string>
#include "types.h"
#include "lexer.h"

struct Binding {
    int scope_depth;
    int slot_index;   
    int storage_class; 
    std::shared_ptr<Type> type;
};

class BindingTable {
private:
    // key: "line:col" -> represent the location message
    std::map<std::string, Binding> table;

    static std::string key_trans(const Token& t) {
        return std::to_string(t.lineNumber) + ":" + std::to_string(t.columnNumber);
    }

public:
    void record(const Token& t, const Binding& b) {
        table[key_trans(t)] = b;
    }

    bool get(const Token& t, Binding& out) const {
        auto it = table.find(key_trans(t));
        if (it == table.end()) return false;
        out = it->second;
        return true;
    }
};

#endif 
