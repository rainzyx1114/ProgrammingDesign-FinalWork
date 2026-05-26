#include "executor.h"
#include "class_model.h"

ExecutorVisitor::ExecutorVisitor(std::shared_ptr<Memory> mem,
                               std::shared_ptr<SymbolTable> sym,
                               std::shared_ptr<TypeSystem> types,
                               std::shared_ptr<ClassModel> cls)
    : memory(mem), symbolTable(sym), typeSystem(types), classModel(cls),
      mode(ExecutionMode::PAUSED), shouldBreak(false), shouldContinue(false), shouldReturn(false), nextLineToExecute(0) {
}

Executor::Executor(std::shared_ptr<Memory> mem,
                   std::shared_ptr<SymbolTable> sym,
                   std::shared_ptr<TypeSystem> types,
                   std::shared_ptr<ClassModel> cls)
    : visitor(std::make_shared<ExecutorVisitor>(mem, sym, types, cls)) {
}

void ExecutorVisitor::executeProgram(const std::shared_ptr<Program>& program) {
    if (program) {
        // Setup global frame and lexical slots before executing the program.
        int totalLevels = symbolTable->getTotalLevels();
        std::vector<int> slotsPerLevel;
        for (int i = 0; i < totalLevels; ++i) slotsPerLevel.push_back(symbolTable->getSlotCountForLevel(i));
        memory->initLexicalFrames(slotsPerLevel);
        auto globalNames = buildCurrentLexicalVariableNames();
        memory->pushFrame("global", globalNames);

        program->accept(*this);
        recordSnapshot("program_end");
        // memory->popFrame();
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
    memory->pushScopeFrame(node.slotCount);

    for (auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
            if (shouldReturn || shouldBreak || shouldContinue) {
                break;
            }
        }
    }

    memory->popScopeFrame();
    symbolTable->exitScope();
    // record scope exit
    recordSnapshot("scope_exit");
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
    int level = symbolTable->getCurrentLevel();
    int slots = symbolTable->getSlotCountForLevel(level);
    memory->pushScopeFrame(slots);
    
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
    
    memory->popScopeFrame();
    symbolTable->exitScope();
    shouldBreak = false;
}

void ExecutorVisitor::visit(ReturnStmt& node) {
    int returnLine = 0;
    if (node.value) {
        node.value->accept(*this);
        returnValue = currentValue;
        // if there is a value expression, use its token line when available
        if (auto var = dynamic_cast<Variable*>(node.value.get())) {
            returnLine = var->name.lineNumber;
        }
    }
    shouldReturn = true;
    recordSnapshot("return", returnLine);
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
    // use binding attached to AST node by analyzer
    if (node.binding.type) {
        currentValue = memory->getByBinding(node.binding.scope_depth, node.binding.slot_index);
    } else {
        currentValue = Value();
    }
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

    // resolve function declaration
    FuncDecl* fdecl = nullptr;
    int callLine = 0;
    if (node.name) {
        if (auto var = dynamic_cast<Variable*>(node.name.get())) {
            fdecl = symbolTable->lookupFunction(var->name.lexeme);
            callLine = var->name.lineNumber;
        }
    }
    if (!fdecl) {
        currentValue = Value();
        return;
    }

    // Call: push frame and init lexical frames for function
    auto callerNames = buildCurrentLexicalVariableNames();
    memory->pushFrame(fdecl->name.lexeme, callerNames);
    int totalLevels = symbolTable->getTotalLevels();
    std::vector<int> slotsPerLevel;
    for (int i = 0; i < totalLevels; ++i) {
        slotsPerLevel.push_back(symbolTable->getSlotCountForLevel(i));
    }
    slotsPerLevel.push_back(fdecl->paramSlotCount);
    memory->initLexicalFrames(slotsPerLevel);

    // write parameters into their bindings and record their names
    for (size_t i = 0; i < fdecl->params.size() && i < args.size(); ++i) {
        if (i < fdecl->param_bindings.size()) {
            Binding b = fdecl->param_bindings[i];
            memory->setByBinding(b.scope_depth, b.slot_index, args[i]);
            memory->setLexicalVariableName(b.scope_depth, b.slot_index, fdecl->params[i].first.lexeme);
        }
    }
    // record entering call after parameters are set
    recordSnapshot("call_enter", callLine);

    // execute body
    bool prevShouldReturn = shouldReturn;
    Value prevReturnValue = returnValue;
    shouldReturn = false;
    returnValue = Value();

    if (fdecl->body) fdecl->body->accept(*this);

    Value result;
    if (shouldReturn) result = returnValue;
    else result = Value();

    shouldReturn = prevShouldReturn;
    returnValue = prevReturnValue;

    // record return from call before popping frame so we capture final frame state
    recordSnapshot("call_return", fdecl->name.lineNumber);
    memory->popFrame();

    currentValue = result;
}

void ExecutorVisitor::visit(MemberAccess& node) {
    if (node.object) node.object->accept(*this);
    // Access member (placeholder)
    currentValue = Value();
}

void ExecutorVisitor::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    // Value arrayVal = currentValue;
    
    if (node.index) node.index->accept(*this);
    // Value indexVal = currentValue;
    
    currentValue = Value();
}

void ExecutorVisitor::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    Value val = currentValue;
    if (node.binding.type) {
        memory->setByBinding(node.binding.scope_depth, node.binding.slot_index, val);
    } else {
        // no binding: ignore
    }
    currentValue = val;
    // record trace
    recordSnapshot("assignment", node.name.lineNumber);
}

// Declaration visitors
void ExecutorVisitor::visit(VarDecl& node) {
    Value initialValue;
    if (node.initializer) {
        node.initializer->accept(*this);
        initialValue = currentValue;
    }
    if (node.binding.type) {
        memory->setByBinding(node.binding.scope_depth, node.binding.slot_index, initialValue);
        memory->setLexicalVariableName(node.binding.scope_depth, node.binding.slot_index, node.name.lexeme);
    } else {
        // no binding: ignore
    }
    // record trace
    recordSnapshot("var_decl", node.name.lineNumber);
}

void ExecutorVisitor::visit(FuncDecl& node) {
}

void ExecutorVisitor::visit(ClassDecl& node) {
}

void ExecutorVisitor::visit(Program& node) {
    for (auto& decl : node.declarations) {
        if (decl) {
            decl->accept(*this);
        }
    }
}

// Step execution methods
// void ExecutorVisitor::stepInto() {
//     // Implementation for single stepping
// }

// void ExecutorVisitor::stepOver() {
//     // Implementation for step over
// }

// void ExecutorVisitor::stepOut() {
//     // Implementation for step out
// }

// void ExecutorVisitor::runUntilBreakpoint(int line) {
//     // Implementation for running to breakpoint
// }

// Helper functions
bool ExecutorVisitor::isTrue(const Value& val) const {
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

std::vector<std::vector<std::string>> ExecutorVisitor::buildCurrentLexicalVariableNames() const {
    return memory->getLexicalVariableNames();
}

std::vector<VariableInfo> ExecutorVisitor::buildVariableInfoForCallFrame(int frameIndex,
                                                           const std::vector<std::vector<Value>>& lexicalFrames,
                                                           const std::vector<std::vector<std::string>>& lexicalNames) const {
    std::vector<VariableInfo> vars;
    for (int depth = 0; depth < (int)lexicalFrames.size(); ++depth) {
        for (int slotIndex = 0; slotIndex < (int)lexicalFrames[depth].size(); ++slotIndex) {
            VariableInfo vi;
            if (depth < (int)lexicalNames.size() && slotIndex < (int)lexicalNames[depth].size()) {
                vi.name = lexicalNames[depth][slotIndex];
            } else {
                Symbol* sym = symbolTable->lookupByBinding(depth, slotIndex);
                if (sym) vi.name = sym->name;
                else vi.name = "slot" + std::to_string(slotIndex);
            }
            
            // Get type from symbol if available
            Symbol* sym = symbolTable->lookupByBinding(depth, slotIndex);
            if (sym && sym->type) {
                vi.type = sym->type->toString();
            } else {
                // Infer type from value if symbol type not available
                const Value& val = lexicalFrames[depth][slotIndex];
                switch (val.type) {
                    case Value::INT:
                        vi.type = "int";
                        break;
                    case Value::FLOAT:
                        vi.type = "float";
                        break;
                    case Value::BOOL:
                        vi.type = "bool";
                        break;
                    case Value::OBJECT_REF:
                        if (val.objectRef) {
                            vi.type = val.objectRef->className;
                        } else {
                            vi.type = "object";
                        }
                        break;
                    case Value::POINTER:
                        vi.type = "pointer";
                        break;
                    case Value::ARRAY:
                        vi.type = "array";
                        break;
                    default:
                        vi.type = "unknown";
                        break;
                }
            }
            
            vi.value = lexicalFrames[depth][slotIndex].toString();
            vars.push_back(vi);
        }
    }
    return vars;
}

// Trace collection helpers
void ExecutorVisitor::recordSnapshot(const std::string& event, int lineNumber) {
    Stepsnapshot snap;
    snap.stepIndex = static_cast<int>(executionTrace.size());
    snap.event = event;
    ExecutionState st;
    auto frame = memory->currentFrame();
    if (frame && lineNumber > 0) {
        frame->lineNumber = lineNumber;
    }
    st.currentLine = frame ? frame->lineNumber : 0;
    st.currentFunction = frame ? frame->functionName : std::string();

    StackTraceView stv;
    const auto& callStack = memory->getCallStack();
    int totalFrames = callStack.size();
    for (int frameIndex = 0; frameIndex < totalFrames; ++frameIndex) {
        auto& f = callStack[frameIndex];
        StackFrameView fv;
        fv.functionName = f->functionName;
        fv.lineNumber = f->lineNumber;
        auto lexicalFrames = memory->getLexicalFramesForCallFrame(frameIndex);
        auto lexicalNames = memory->getLexicalVariableNamesForCallFrame(frameIndex);
        int callerDepth = f->lexicalVariableNames.size();
        if (callerDepth > 0 && callerDepth < (int)lexicalFrames.size()) {
            lexicalFrames = std::vector<std::vector<Value>>(lexicalFrames.begin() + callerDepth, lexicalFrames.end());
        }
        if (callerDepth > 0 && callerDepth < (int)lexicalNames.size()) {
            lexicalNames = std::vector<std::vector<std::string>>(lexicalNames.begin() + callerDepth, lexicalNames.end());
        }
        fv.variables = buildVariableInfoForCallFrame(frameIndex, lexicalFrames, lexicalNames);
        stv.frames.push_back(fv);
    }
    if (totalFrames == 0) {
        StackFrameView fv;
        fv.functionName = "global";
        fv.lineNumber = st.currentLine;
        auto slots = memory->getCurrentLexicalSlots();
        auto names = memory->getLexicalVariableNames();
        for (size_t i = 0; i < slots.size(); ++i) {
            VariableInfo vi;
            if (0 < (int)names.size() && i < names[0].size()) {
                vi.name = names[0][i];
            } else {
                vi.name = std::string("slot") + std::to_string(i);
            }
            
            // Get type from symbol table if available
            Symbol* sym = symbolTable->lookupByBinding(0, i);
            if (sym && sym->type) {
                vi.type = sym->type->toString();
            } else {
                // Infer type from value
                const Value& val = slots[i];
                switch (val.type) {
                    case Value::INT:
                        vi.type = "int";
                        break;
                    case Value::FLOAT:
                        vi.type = "float";
                        break;
                    case Value::BOOL:
                        vi.type = "bool";
                        break;
                    case Value::OBJECT_REF:
                        if (val.objectRef) {
                            vi.type = val.objectRef->className;
                        } else {
                            vi.type = "object";
                        }
                        break;
                    case Value::POINTER:
                        vi.type = "pointer";
                        break;
                    case Value::ARRAY:
                        vi.type = "array";
                        break;
                    default:
                        vi.type = "unknown";
                        break;
                }
            }
            
            vi.value = slots[i].toString();
            fv.variables.push_back(vi);
        }
        stv.frames.push_back(fv);
    }

    st.stackTrace = stv;

    std::vector<ObjectView> objs;
    for (auto& kv : memory->getHeap()) {
        ObjectView ov;
        ov.objectId = kv.first;
        ov.className = kv.second->className;
        ov.baseClass = "";
        
        // Get class definition to retrieve member types
        ClassDef* classDef = classModel->getClass(kv.second->className);
        
        for (auto& m : kv.second->members) {
            MemberInfo mi;
            mi.name = m.first;
            mi.isMethod = false;
            mi.value = m.second.toString();
            
            // Get type from class definition
            if (classDef) {
                std::shared_ptr<Type> memberType = classDef->getMemberType(m.first);
                if (memberType) {
                    mi.type = memberType->toString();
                } else {
                    mi.type = "unknown";
                }
            } else {
                mi.type = "unknown";
            }
            
            ov.members.push_back(mi);
        }
        objs.push_back(ov);
    }
    st.objectsOnHeap = objs;
    snap.state = st;
    executionTrace.push_back(snap);
}

void Executor::executeProgram(const std::shared_ptr<Program>& program) {
    visitor->executeProgram(program);
}

Value Executor::evaluateExpression(const std::shared_ptr<Expr>& expr) {
    return visitor->evaluateExpression(expr);
}

// void Executor::stepInto() {
//     visitor->stepInto();
// }

// void Executor::stepOver() {
//     visitor->stepOver();
// }

// void Executor::stepOut() {
//     visitor->stepOut();
// }

// void Executor::runUntilBreakpoint(int line) {
//     visitor->runUntilBreakpoint(line);
// }

std::shared_ptr<Memory> Executor::getMemory() const {
    return visitor->getMemory();
}

ExecutionMode Executor::getMode() const {
    return visitor->getMode();
}

int Executor::getNextLine() const {
    return visitor->getNextLine();
}

std::vector<Stepsnapshot> Executor::getExecutionTrace() const {
    return visitor->getExecutionTrace();
}
