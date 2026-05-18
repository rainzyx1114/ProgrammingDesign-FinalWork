#include "executor.h"
#include "class_model.h"

ExecutorVisitor::ExecutorVisitor(std::shared_ptr<Memory> mem,
                               std::shared_ptr<SymbolTable> sym,
                               std::shared_ptr<TypeSystem> types,
                               std::shared_ptr<ClassModel> cls)
    : memory(mem), symbolTable(sym), typeSystem(types), classModel(cls),
      mode(ExecutionMode::PAUSED), shouldBreak(false), shouldContinue(false),
      shouldReturn(false), nextLineToExecute(0) {
}

void ExecutorVisitor::executeProgram(const std::shared_ptr<Program>& program) {
    if (program) {
        program->accept(*this);
    }
}

Value ExecutorVisitor::evaluateExpression(const std::shared_ptr<Expr>& expr) {
    if (expr) {
        expr->accept(*this);
        return currentValue;
    }
    return Value();
}

// Statement visitors
void ExecutorVisitor::visit(ExprStmt& node) {
    if (node.expr) {
        node.expr->accept(*this);
    }
}

void ExecutorVisitor::visit(Block& node) {
    symbolTable->enterScope();
    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
            if (shouldReturn || shouldBreak || shouldContinue) {
                break;
            }
        }
    }
    symbolTable->exitScope();
}

void ExecutorVisitor::visit(IfStmt& node) {
    if (node.condition) {
        node.condition->accept(*this);
        if (isTrue(currentValue)) {
            if (node.thenBranch) {
                node.thenBranch->accept(*this);
            }
        } else if (node.elseBranch) {
            node.elseBranch->accept(*this);
        }
    }
}

void ExecutorVisitor::visit(WhileStmt& node) {
    while (!shouldReturn && !shouldBreak) {
        if (node.condition) {
            node.condition->accept(*this);
            if (!isTrue(currentValue)) {
                break;
            }
        }
        
        if (node.body) {
            node.body->accept(*this);
        }
        
        if (shouldContinue) {
            shouldContinue = false;
            continue;
        }
    }
    shouldBreak = false;
}

void ExecutorVisitor::visit(ForStmt& node) {
    symbolTable->enterScope();
    
    if (node.init) {
        node.init->accept(*this);
    }
    
    while (!shouldReturn && !shouldBreak) {
        if (node.condition) {
            node.condition->accept(*this);
            if (!isTrue(currentValue)) {
                break;
            }
        }
        
        if (node.body) {
            node.body->accept(*this);
        }
        
        if (shouldContinue) {
            shouldContinue = false;
        }
        
        if (node.update) {
            node.update->accept(*this);
        }
    }
    
    symbolTable->exitScope();
    shouldBreak = false;
}

void ExecutorVisitor::visit(ReturnStmt& node) {
    if (node.value) {
        node.value->accept(*this);
        returnValue = currentValue;
    }
    shouldReturn = true;
}

// Expression visitors
void ExecutorVisitor::visit(BinaryOp& node) {
    if (node.left) node.left->accept(*this);
    Value leftVal = currentValue;
    
    if (node.right) node.right->accept(*this);
    Value rightVal = currentValue;
    
    currentValue = applyBinaryOp(node.op.lexeme, leftVal, rightVal);
}

void ExecutorVisitor::visit(LogicalOp& node) {
    if (node.left) node.left->accept(*this);
    Value leftVal = currentValue;
    
    // Short-circuit evaluation for logical operators
    if (node.op.lexeme == "&&") {
        if (!isTrue(leftVal)) {
            currentValue = Value(false);
            return;
        }
    } else if (node.op.lexeme == "||") {
        if (isTrue(leftVal)) {
            currentValue = Value(true);
            return;
        }
    }
    
    if (node.right) node.right->accept(*this);
    Value rightVal = currentValue;
    
    currentValue = applyBinaryOp(node.op.lexeme, leftVal, rightVal);
}

void ExecutorVisitor::visit(UnaryOp& node) {
    if (node.operand) node.operand->accept(*this);
    currentValue = applyUnaryOp(node.op.lexeme, currentValue);
}

void ExecutorVisitor::visit(Literal& node) {
    if (node.value.type == TokenType::NUMBER) {
        try {
            if (node.value.lexeme.find('.') != std::string::npos) {
                currentValue = Value(static_cast<float>(std::stod(node.value.lexeme)));
            } else {
                currentValue = Value(std::stoi(node.value.lexeme));
            }
        } catch (...) {
            currentValue = Value(0);
        }
    } else if (node.value.type == TokenType::TRUE) {
        currentValue = Value(true);
    } else if (node.value.type == TokenType::FALSE) {
        currentValue = Value(false);
    } else {
        currentValue = Value();
    }
}

void ExecutorVisitor::visit(Variable& node) {
    // Look up variable in symbol table and memory
    // currentValue = memory->get(symbolTable->getAddress(node.name.lexeme));
    currentValue = Value(); // Placeholder
}

void ExecutorVisitor::visit(FunctionCall& node) {
    // Evaluate arguments
    std::vector<Value> args;
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
            args.push_back(currentValue);
        }
    }
    
    // Call function (placeholder implementation)
    currentValue = Value(); // Placeholder for function result
}

void ExecutorVisitor::visit(MemberAccess& node) {
    if (node.object) node.object->accept(*this);
    // Access member (placeholder)
    currentValue = Value();
}

void ExecutorVisitor::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    Value arrayVal = currentValue;
    
    if (node.index) node.index->accept(*this);
    Value indexVal = currentValue;
    
    // Array access (placeholder)
    currentValue = Value();
}

void ExecutorVisitor::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    Value val = currentValue;
    
    // Store value in variable
    // memory->set(symbolTable->getAddress(node.name.lexeme), val);
    currentValue = val; // Assignment returns the assigned value
}

// Declaration visitors
void ExecutorVisitor::visit(VarDecl& node) {
    Value initialValue;
    if (node.initializer) {
        node.initializer->accept(*this);
        initialValue = currentValue;
    }
    
    // Declare variable in symbol table and allocate memory
    // auto address = memory->allocate(node.type);
    // symbolTable->declare(node.name.lexeme, address);
    // if (node.initializer) memory->set(address, initialValue);
}

void ExecutorVisitor::visit(FuncDecl& node) {
    // Store function definition
    // symbolTable->declareFunction(node.name.lexeme, node);
}

void ExecutorVisitor::visit(ClassDecl& node) {
    // Define class
    // classModel->addClass(node.name.lexeme, node);
}

void ExecutorVisitor::visit(Program& node) {
    for (auto& decl : node.declarations) {
        if (decl) {
            decl->accept(*this);
        }
    }
}

// Step execution methods
void ExecutorVisitor::stepInto() {
    // Implementation for single stepping
}

void ExecutorVisitor::stepOver() {
    // Implementation for step over
}

void ExecutorVisitor::stepOut() {
    // Implementation for step out
}

void ExecutorVisitor::runUntilBreakpoint(int line) {
    // Implementation for running to breakpoint
}

// Helper functions
bool ExecutorVisitor::isTrue(const Value& val) const {
    // Implementation depends on Value class
    return val.toBool();
}

Value ExecutorVisitor::applyBinaryOp(const std::string& op, const Value& left, const Value& right) {
    // Implementation of binary operations
    if (op == "+") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() + right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() + right.toFloat());
        }
    } else if (op == "-") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() - right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() - right.toFloat());
        }
    } else if (op == "*") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() * right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() * right.toFloat());
        }
    } else if (op == "/") {
        if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() / right.toFloat());
        } else {
            return Value(left.toInt() / right.toInt());
        }
    } else if (op == "==") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() == right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() == right.toFloat());
        } else if (left.type == Value::BOOL && right.type == Value::BOOL) {
            return Value(left.toBool() == right.toBool());
        }
    } else if (op == "!=") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() != right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() != right.toFloat());
        } else if (left.type == Value::BOOL && right.type == Value::BOOL) {
            return Value(left.toBool() != right.toBool());
        }
    } else if (op == "<") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() < right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() < right.toFloat());
        }
    } else if (op == "<=") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() <= right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() <= right.toFloat());
        }
    } else if (op == ">") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() > right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() > right.toFloat());
        }
    } else if (op == ">=") {
        if (left.type == Value::INT && right.type == Value::INT) {
            return Value(left.toInt() >= right.toInt());
        } else if (left.type == Value::FLOAT || right.type == Value::FLOAT) {
            return Value(left.toFloat() >= right.toFloat());
        }
    } else if (op == "&&") {
        return Value(left.toBool() && right.toBool());
    } else if (op == "||") {
        return Value(left.toBool() || right.toBool());
    }
    
    return Value(); // Default case
}

Value ExecutorVisitor::applyUnaryOp(const std::string& op, const Value& val) {
    // Implementation of unary operations
    if (op == "-") {
        if (val.type == Value::INT) {
            return Value(-val.toInt());
        } else if (val.type == Value::FLOAT) {
            return Value(-val.toFloat());
        }
    } else if (op == "!") {
        return Value(!val.toBool());
    }
    
    return val; // Default case
}

// Executor class implementation
Executor::Executor(std::shared_ptr<Memory> mem,
                   std::shared_ptr<SymbolTable> sym,
                   std::shared_ptr<TypeSystem> types,
                   std::shared_ptr<ClassModel> cls) {
    visitor = std::make_shared<ExecutorVisitor>(mem, sym, types, cls);
}

void Executor::executeProgram(const std::shared_ptr<Program>& program) {
    visitor->executeProgram(program);
}

Value Executor::evaluateExpression(const std::shared_ptr<Expr>& expr) {
    return visitor->evaluateExpression(expr);
}

void Executor::stepInto() {
    visitor->stepInto();
}

void Executor::stepOver() {
    visitor->stepOver();
}

void Executor::stepOut() {
    visitor->stepOut();
}

void Executor::runUntilBreakpoint(int line) {
    visitor->runUntilBreakpoint(line);
}

std::shared_ptr<Memory> Executor::getMemory() const {
    return visitor->getMemory();
}

ExecutionMode Executor::getMode() const {
    return visitor->getMode();
}

int Executor::getNextLine() const {
    return visitor->getNextLine();
}
