#include "symbol_table.h"
#include "ast.h"
#include <stdexcept>

Symbol::Symbol(const std::string& n, std::shared_ptr<Type> t, int level)
    : name(n), type(t), scopeLevel(level), isInitialized(false) {
}

Symbol::Symbol(const std::string& n, std::shared_ptr<Type> t, int level, bool initialized)
    : name(n), type(t), scopeLevel(level), isInitialized(initialized) {
    }

SymbolTable::SymbolTable()
    : currentLevel(0) {
    scopes.push_back(std::map<std::string, Symbol>());
    nextSlotIndexPerScope.push_back(0);
}

void SymbolTable::enterScope() {
    scopes.push_back(std::map<std::string, Symbol>());
    nextSlotIndexPerScope.push_back(0);
    currentLevel ++;
}

void SymbolTable::exitScope() {
    if (scopes.empty()) {return;}
    scopes.pop_back();
    nextSlotIndexPerScope.pop_back();
    currentLevel --;
}

Binding SymbolTable::declare(const std::string& name, std::shared_ptr<Type> type, const Token& token) {
    int slot = nextSlotIndexPerScope.back();
    nextSlotIndexPerScope.back() = slot + 1;
    scopes.back()[name] = Symbol(name, type, currentLevel);

    Binding b;
    b.scope_depth = currentLevel;
    b.slot_index = slot;
    b.storage_class = 0;
    b.type = type;
    // record binding into symbol as well
    scopes.back()[name].binding = b;
    return b;
}

Symbol* SymbolTable::lookup(const std::string& name) {
    int l = currentLevel;
    while(l >= 0) {
        if (lookupLocal(name, l)) {
            return lookupLocal(name, l);
        }
        l --;
    }
    return nullptr;
}

Symbol* SymbolTable::lookupLocal(const std::string& name, int level) {
    if(level < 0 || level >= (int)scopes.size()) return nullptr;
    if(scopes[level].find(name) != scopes[level].end()) {
        return &scopes[level][name];
    }
    return nullptr;
}

void SymbolTable::markInitialized(const std::string& name) {
    auto s = lookup(name);
    if (s) s->isInitialized = true;
}

bool SymbolTable::isDeclared(const std::string& name) {
    return lookup(name) != nullptr;
}

Symbol* SymbolTable::lookupByBinding(int scope_level, int slot_index) {
    if (scope_level < 0 || scope_level >= (int)scopes.size()) return nullptr;
    for (auto& kv : scopes[scope_level]) {
        if (kv.second.binding.slot_index == slot_index) {
            return &kv.second;
        }
    }
    return nullptr;
}

void SymbolTable::declareFunction(const std::string& name, FuncDecl* decl) {
    functions[name] = decl;
}

FuncDecl* SymbolTable::lookupFunction(const std::string& name) {
    auto it = functions.find(name);
    if (it != functions.end()) return it->second;
    return nullptr;
}
