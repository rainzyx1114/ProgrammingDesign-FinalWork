#include "symbol_table.h"

Symbol::Symbol(const std::string& n, std::shared_ptr<Type> t, int level)
    : name(n), type(t), scopeLevel(level), isInitialized(false) {
}

SymbolTable::SymbolTable()
    : currentLevel(0) {
    scopes.push_back(std::map<std::string, Symbol>());
}

void SymbolTable::enterScope() {
    // Implementation
}

void SymbolTable::exitScope() {
    // Implementation
}

void SymbolTable::declare(const std::string& name, std::shared_ptr<Type> type) {
    // Implementation
}

Symbol* SymbolTable::lookup(const std::string& name) {
    // Implementation
    return nullptr;
}

Symbol* SymbolTable::lookupLocal(const std::string& name) {
    // Implementation
    return nullptr;
}

void SymbolTable::markInitialized(const std::string& name) {
    // Implementation
}

bool SymbolTable::isDeclared(const std::string& name) {
    // Implementation
    return false;
}
