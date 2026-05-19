#include "symbol_table.h"

Symbol::Symbol(const std::string& n, std::shared_ptr<Type> t, int level)
    : name(n), type(t), scopeLevel(level), isInitialized(false) {
}

Symbol::Symbol(const std::string& n, std::shared_ptr<Type> t, int level, bool initialized)
    : name(n), type(t), scopeLevel(level), isInitialized(initialized) {
    }

SymbolTable::SymbolTable()
    : currentLevel(0) {
    scopes.push_back(std::map<std::string, Symbol>());
}

void SymbolTable::enterScope() {
    scopes.push_back(std::map<std::string, Symbol>());
    currentLevel ++;
}

void SymbolTable::exitScope() {
    if (scopes.empty()) {return;}
    scopes.pop_back();
    currentLevel --;
}

void SymbolTable::declare(const std::string& name, std::shared_ptr<Type> type) {
    scopes.back()[name] = Symbol(name, type, currentLevel);
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
    if(scopes[level].find(name) != scopes[level].end()) {
        return &scopes[level][name];
    }
    return nullptr;
}

void SymbolTable::markInitialized(const std::string& name) {
    lookup(name)->isInitialized = true;
}

bool SymbolTable::isDeclared(const std::string& name) {
    return lookup(name) != nullptr;
}
