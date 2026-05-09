#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "types.h"

class Symbol {
public:
    std::string name;
    std::shared_ptr<Type> type;
    int scopeLevel;
    bool isInitialized;
    
    Symbol(const std::string& n, std::shared_ptr<Type> t, int level);
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol>> scopes;
    int currentLevel;
    
public:
    SymbolTable();
    
    void enterScope();
    void exitScope();
    void declare(const std::string& name, std::shared_ptr<Type> type);
    Symbol* lookup(const std::string& name);
    Symbol* lookupLocal(const std::string& name);
    void markInitialized(const std::string& name);
    bool isDeclared(const std::string& name);
    
    int getCurrentLevel() const { return currentLevel; }
};

#endif
