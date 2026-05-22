#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "types.h"
#include "lexer.h"
#include "binding.h"
class FuncDecl; // forward declare

class Symbol {
public:
    std::string name;
    std::shared_ptr<Type> type;
    int scopeLevel;
    bool isInitialized;
    Binding binding; // binding assigned at declaration
    
    Symbol() = default;
    Symbol(const std::string& n, std::shared_ptr<Type> t, int level);
    Symbol(const std::string& n, std::shared_ptr<Type> t, int level, bool initialized);
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol>> scopes;
    // track next slot index per lexical scope
    std::vector<int> nextSlotIndexPerScope;
    int currentLevel;

public:
    SymbolTable();
    
    void enterScope();
    void exitScope();
    
    Binding declare(const std::string& name, std::shared_ptr<Type> type, const Token& token);
    Symbol* lookup(const std::string& name);
    Symbol* lookupLocal(const std::string& name, int level);
    void markInitialized(const std::string& name);
    bool isDeclared(const std::string& name);
    
    int getCurrentLevel() const { return currentLevel; }
    int getSlotCountForLevel(int level) const { if (level < 0 || level >= (int)nextSlotIndexPerScope.size()) return 0; return nextSlotIndexPerScope[level]; }
    int getTotalLevels() const { return (int)nextSlotIndexPerScope.size(); }

    // Function declarations
    void declareFunction(const std::string& name, FuncDecl* decl);
    FuncDecl* lookupFunction(const std::string& name);

private:
    std::map<std::string, FuncDecl*> functions;
};

#endif
