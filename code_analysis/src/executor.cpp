#include "executor.h"
#include "class_model.h"
#include <set>

ExecutorVisitor::ExecutorVisitor(std::shared_ptr<Memory> mem,
                               std::shared_ptr<SymbolTable> sym,
                               std::shared_ptr<ClassModel> cls)
    : memory(mem), symbolTable(sym), classModel(cls),
      mode(ExecutionMode::PAUSED), shouldBreak(false), shouldContinue(false), shouldReturn(false), nextLineToExecute(0),
      currentThis(nullptr) {
}

Executor::Executor(std::shared_ptr<Memory> mem,
                   std::shared_ptr<SymbolTable> sym,
                   std::shared_ptr<ClassModel> cls)
    : visitor(std::make_shared<ExecutorVisitor>(mem, sym, cls)) {
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

        // Execute global declarations / statements first.
        bool hasMain = symbolTable->lookupFunction("main") != nullptr;
        if (hasMain) {
            for (auto& decl : program->declarations) {
                if (!decl) continue;
                if (std::dynamic_pointer_cast<FuncDecl>(decl) || std::dynamic_pointer_cast<ClassDecl>(decl)) {
                    continue;
                }
                decl->accept(*this);
            }

            // Call main() as program entry point.
            auto mainVar = std::make_shared<Variable>(Token(TokenType::IDENTIFIER, "main", 0, 0));
            auto mainCall = std::make_shared<FunctionCall>(mainVar, std::vector<std::shared_ptr<Expr>>());
            mainCall->accept(*this);
        } else {
            program->accept(*this);
        }

        recordSnapshot("program_end");
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

    // record scope exit BEFORE popping so snapshot captures local variables
    recordSnapshot("scope_exit");
    memory->popScopeFrame();
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
    if (node.binding.scope_depth >= 0) {
        currentValue = memory->getByBinding(node.binding.scope_depth, node.binding.slot_index);
    } else if (currentThis) {
        // Fallback: resolve as member of the current 'this' object
        currentValue = currentThis->getMember(node.name.lexeme);
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

    // Resolve function declaration
    FuncDecl* fdecl = nullptr;
    int callLine = 0;
    std::string actualClassName; // for virtual dispatch
    std::shared_ptr<Object> methodObject; // for "this" context capture

    if (node.name) {
        if (auto ma = dynamic_cast<MemberAccess*>(node.name.get())) {
            // Method call: obj.method() or ptr->method()
            // Evaluate the object to get its runtime type
            ma->object->accept(*this);
            Value objVal = currentValue;

            // Determine the actual class name from the runtime object
            if (ma->isPointer) {
                int objId = objVal.getPointerId();
                auto obj = memory->getObjectById(objId);
                if (obj) {
                    actualClassName = obj->className;
                    methodObject = obj;
                }
            } else {
                if (objVal.objectRef) {
                    actualClassName = objVal.objectRef->className;
                    methodObject = objVal.objectRef;
                }
            }

            if (!actualClassName.empty()) {
                // Check for virtual dispatch
                std::string resolved = classModel->resolveVirtualMethod(actualClassName, ma->member);
                if (!resolved.empty()) {
                    // Virtual method: look up the resolved implementation
                    fdecl = symbolTable->lookupFunction(resolved);
                }
                if (!fdecl) {
                    // Non-virtual or not found via virtual dispatch:
                    // look up ClassName::methodName
                    fdecl = symbolTable->lookupFunction(actualClassName + "::" + ma->member);
                }
                // Inheritance chain search for non-virtual inherited methods
                if (!fdecl) {
                    std::string baseClass = classModel->getBaseClass(actualClassName);
                    while (!baseClass.empty()) {
                        fdecl = symbolTable->lookupFunction(baseClass + "::" + ma->member);
                        if (fdecl) break;
                        baseClass = classModel->getBaseClass(baseClass);
                    }
                }
                // Also try the method name directly (for global-like methods)
                if (!fdecl) {
                    fdecl = symbolTable->lookupFunction(ma->member);
                }
            }
            if (auto var = dynamic_cast<Variable*>(ma->object.get())) {
                callLine = var->name.lineNumber;
            }
        } else if (auto var = dynamic_cast<Variable*>(node.name.get())) {
            // Standalone function call: foo()
            fdecl = symbolTable->lookupFunction(var->name.lexeme);
            callLine = var->name.lineNumber;

            // Bare call fallback: if inside a method and bare name not found,
            // try dispatch through 'this'
            if (!fdecl && currentThis && !currentClassName.empty()) {
                std::string resolved = classModel->resolveVirtualMethod(
                    currentClassName, var->name.lexeme);
                if (!resolved.empty()) {
                    fdecl = symbolTable->lookupFunction(resolved);
                    if (fdecl) {
                        methodObject = currentThis;
                        actualClassName = currentClassName;
                    }
                }
                if (!fdecl) {
                    fdecl = symbolTable->lookupFunction(
                        currentClassName + "::" + var->name.lexeme);
                    if (fdecl) {
                        methodObject = currentThis;
                        actualClassName = currentClassName;
                    }
                }
            }
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

    // Save and set "this" context for method calls
    auto savedThis = currentThis;
    auto savedClassName = currentClassName;
    if (methodObject && !actualClassName.empty()) {
        currentThis = methodObject;
        currentClassName = actualClassName;
    }

    if (fdecl->body) fdecl->body->accept(*this);

    // Restore "this" context
    currentThis = savedThis;
    currentClassName = savedClassName;

    Value result;
    if (shouldReturn) result = returnValue;
    else result = Value();

    shouldReturn = prevShouldReturn;
    returnValue = prevReturnValue;

    // record return from call before popping frame so we capture final frame state
    recordSnapshot("call_return", fdecl->name.lineNumber, &result);
    memory->popFrame();

    currentValue = result;
}

void ExecutorVisitor::visit(MemberAccess& node) {
    if (node.object) node.object->accept(*this);
    Value objVal = currentValue;

    if (node.isPointer) {
        // objVal is a POINTER; dereference to get the heap object
        int objId = objVal.getPointerId();
        auto obj = memory->getObjectById(objId);
        if (obj) {
            currentValue = obj->getMember(node.member);
        } else {
            currentValue = Value();
        }
    } else {
        // objVal should be an OBJECT_REF
        if (objVal.objectRef) {
            currentValue = objVal.objectRef->getMember(node.member);
        } else {
            currentValue = Value();
        }
    }
}

void ExecutorVisitor::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    Value arrVal = currentValue;

    if (node.index) node.index->accept(*this);
    int idx = currentValue.toInt();

    // Arrays are stored as heap objects with numeric member names
    if (arrVal.type == Value::OBJECT_REF && arrVal.objectRef) {
        currentValue = arrVal.objectRef->getMember(std::to_string(idx));
    } else {
        currentValue = Value();
    }
}

void ExecutorVisitor::visit(NewExpr& node) {
    // Create an instance with default-initialized members and put directly on heap
    auto instance = classModel->createInstance(node.className);
    int objId = memory->putOnHeap(instance);
    // Return a pointer value (the heap object ID)
    currentValue = Value::pointerFromId(objId);
    recordSnapshot("var_decl");
}

void ExecutorVisitor::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    Value val = currentValue;

    if (node.target) {
        // l-value is a member access or array access
        if (auto ma = dynamic_cast<MemberAccess*>(node.target.get())) {
            // Evaluate the object expression
            ma->object->accept(*this);
            Value objVal = currentValue;

            if (ma->isPointer) {
                int objId = objVal.getPointerId();
                auto obj = memory->getObjectById(objId);
                if (obj) {
                    obj->setMember(ma->member, val);
                }
            } else {
                if (objVal.objectRef) {
                    objVal.objectRef->setMember(ma->member, val);
                }
            }
        } else if (auto aa = dynamic_cast<ArrayAccess*>(node.target.get())) {
            // Evaluate the array expression
            aa->array->accept(*this);
            Value arrVal = currentValue;

            // Evaluate the index
            aa->index->accept(*this);
            int idx = currentValue.toInt();

            if (arrVal.type == Value::OBJECT_REF && arrVal.objectRef) {
                arrVal.objectRef->setMember(std::to_string(idx), val);
            }
        }
    } else if (node.binding.scope_depth >= 0) {
        // Simple variable assignment (existing path)
        memory->setByBinding(node.binding.scope_depth, node.binding.slot_index, val);
    } else if (currentThis && !node.name.lexeme.empty()) {
        // Fallback: set member on the current 'this' object
        currentThis->setMember(node.name.lexeme, val);
    }

    currentValue = val;
    // record trace
    int lineNum = node.name.lineNumber;
    if (lineNum == 0 && node.target) {
        // Try to get line from the target expression
        if (auto ma = dynamic_cast<MemberAccess*>(node.target.get())) {
            if (auto var = dynamic_cast<Variable*>(ma->object.get())) {
                lineNum = var->name.lineNumber;
            }
        }
    }
    recordSnapshot("assignment", lineNum);
}

// Declaration visitors
void ExecutorVisitor::visit(VarDecl& node) {
    Value initialValue;
    if (node.initializer) {
        node.initializer->accept(*this);
        initialValue = currentValue;
    }

    // Handle array type: allocate heap storage for the array
    if (node.type) {
        if (auto arrType = dynamic_cast<ArrayType*>(node.type.get())) {
            int size = arrType->size;
            if (size <= 0) size = 1; // default size for unspecified arrays
            auto arrObj = memory->createArray(size);
            initialValue = Value(arrObj);
        } else if (auto classType = dynamic_cast<ClassType*>(node.type.get())) {
            // Handle class type: create instance with default-initialized members
            if (classModel->isDefined(classType->className)) {
                auto instance = classModel->createInstance(classType->className);
                initialValue = Value(instance);
            }
        }
    }

    if (node.binding.scope_depth >= 0) {
        memory->setByBinding(node.binding.scope_depth, node.binding.slot_index, initialValue);
        memory->setLexicalVariableName(node.binding.scope_depth, node.binding.slot_index, node.name.lexeme);
        // Store declared type for later snapshot use (symbol table scopes may be gone)
        if (node.type) {
            memory->setLexicalVariableType(node.binding.scope_depth, node.binding.slot_index, node.type->toString());
        }
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
    } else if (op == "*") {
        // Dereference: if val is POINTER, retrieve the heap object
        if (val.type == Value::POINTER) {
            int objId = val.getPointerId();
            auto obj = memory->getObjectById(objId);
            if (obj) {
                Value result(obj);
                result.type = Value::OBJECT_REF;
                return result;
            }
        }
        return Value();
    } else if (op == "&") {
        // Address-of: for objects, return the pointer (heap ID)
        if (val.type == Value::OBJECT_REF && val.objectRef) {
            // Search both heap and pointerTargets for an existing registration
            int objId = memory->findObjectId(val.objectRef);
            if (objId >= 0) {
                return Value::pointerFromId(objId);
            }
            // Stack object not yet registered: register it as a pointer target
            objId = memory->registerForPointer(val.objectRef);
            return Value::pointerFromId(objId);
        }
        return Value();
    }

    return val; // Default case
}

std::vector<std::vector<std::string>> ExecutorVisitor::buildCurrentLexicalVariableNames() const {
    return memory->getLexicalVariableNames();
}

std::vector<VariableInfo> ExecutorVisitor::buildVariableInfoForCallFrame(int frameIndex,
                                                           const std::vector<std::vector<Value>>& lexicalFrames,
                                                           const std::vector<std::vector<std::string>>& lexicalNames,
                                                           const std::vector<std::vector<std::string>>& lexicalTypes) const {
    std::vector<VariableInfo> vars;
    for (int depth = 0; depth < (int)lexicalFrames.size(); ++depth) {
        for (int slotIndex = 0; slotIndex < (int)lexicalFrames[depth].size(); ++slotIndex) {
            VariableInfo vi;
            if (depth < (int)lexicalNames.size() && slotIndex < (int)lexicalNames[depth].size()) {
                vi.name = lexicalNames[depth][slotIndex];
            } else {
                Symbol* sym = symbolTable->lookupByBinding(depth, slotIndex);
                if (sym) vi.name = sym->name;
            }

            if (isAutoGenerated(vi.name)) {
                continue;
            }

            // Get type from the trimmed type frames (already sliced by callerDepth)
            if (depth < (int)lexicalTypes.size() && slotIndex < (int)lexicalTypes[depth].size()) {
                vi.type = lexicalTypes[depth][slotIndex];
            }
            if (vi.type.empty()) {
                vi.type = inferValueType(lexicalFrames[depth][slotIndex]);
            }

            vi.value = lexicalFrames[depth][slotIndex].toString();
            vars.push_back(vi);
        }
    }
    return vars;
}

std::string ExecutorVisitor::inferValueType(const Value& value) const {
    switch (value.type) {
        case Value::INT:
            return "int";
        case Value::FLOAT:
            return "float";
        case Value::BOOL:
            return "bool";
        case Value::OBJECT_REF:
            if (value.objectRef) {
                return value.objectRef->className;
            } else {
                return "object";
            }
        case Value::POINTER:
            return "pointer";
        default:
            return "unknown";
    }
}

VariableInfo ExecutorVisitor::buildReturnValueInfo(const Value v) {
    return VariableInfo("return", inferValueType(v), v.toString());
}

bool ExecutorVisitor::isAutoGenerated(const std::string& name) const {
    if (name.empty()) return true;
    if (name.find("slot") != 0 || name.size() == 4) return false;
    for (size_t i = 4; i < name.size(); ++i) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
}

ObjectView ExecutorVisitor::buildObjectViewFromObject(
        const std::string& objectId,
        const std::shared_ptr<Object>& obj) const {
    ObjectView ov;
    ov.objectId = objectId;
    ov.className = obj->className;

    ClassDef* classDef = classModel->getClass(obj->className);
    if (classDef) {
        ov.baseClass = classDef->baseClass;
        // Build vtable for this object's class
        const ClassDef* current = classDef;
        while (current) {
            for (auto& methodPair : current->methods) {
                if (methodPair.second.isVirtual && ov.vtable.find(methodPair.first) == ov.vtable.end()) {
                    ov.vtable[methodPair.first] = current->name + "::" + methodPair.first;
                }
            }
            if (current->baseClass.empty()) break;
            current = classModel->getClass(current->baseClass);
        }
    } else {
        ov.baseClass = "";
    }

    for (auto& m : obj->members) {
        MemberInfo mi;
        mi.name = m.first;
        mi.isMethod = false;
        mi.value = m.second.toString();

        // Get type and access level from class definition
        if (classDef) {
            std::shared_ptr<Type> memberType = classDef->getMemberType(m.first);
            if (memberType) {
                mi.type = memberType->toString();
            } else {
                mi.type = "unknown";
            }
            mi.accessLevel = accessLevelToString(classDef->getMemberAccess(m.first));
        } else {
            mi.type = "unknown";
            mi.accessLevel = "private";
        }

        ov.members.push_back(mi);
    }
    return ov;
}

// Trace collection helpers
void ExecutorVisitor::recordSnapshot(const std::string& event, int lineNumber, const Value* returnedValue) {
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
        auto lexicalTypes = memory->getLexicalVariableTypesForCallFrame(frameIndex);
        int callerDepth = f->lexicalVariableNames.size();
        if (callerDepth > 0 && callerDepth < (int)lexicalFrames.size()) {
            lexicalFrames = std::vector<std::vector<Value>>(lexicalFrames.begin() + callerDepth, lexicalFrames.end());
        }
        if (callerDepth > 0 && callerDepth < (int)lexicalNames.size()) {
            lexicalNames = std::vector<std::vector<std::string>>(lexicalNames.begin() + callerDepth, lexicalNames.end());
        }
        if (callerDepth > 0 && callerDepth < (int)lexicalTypes.size()) {
            lexicalTypes = std::vector<std::vector<std::string>>(lexicalTypes.begin() + callerDepth, lexicalTypes.end());
        }
        fv.variables = buildVariableInfoForCallFrame(frameIndex, lexicalFrames, lexicalNames, lexicalTypes);
        if (returnedValue && frameIndex == totalFrames - 1) {
            fv.variables.push_back(buildReturnValueInfo(*returnedValue));
        }

        // Build stack-local objects for this frame
        {
            std::vector<ObjectView> stackObjs;
            std::set<std::shared_ptr<Object>> seenObjects;
            for (int depth = 0; depth < (int)lexicalFrames.size(); ++depth) {
                for (int slotIndex = 0; slotIndex < (int)lexicalFrames[depth].size(); ++slotIndex) {
                    const Value& val = lexicalFrames[depth][slotIndex];
                    if (val.type != Value::OBJECT_REF || !val.objectRef) continue;

                    auto obj = val.objectRef;
                    if (seenObjects.find(obj) != seenObjects.end()) continue;
                    seenObjects.insert(obj);

                    // Get variable name for ID generation
                    std::string varName;
                    if (depth < (int)lexicalNames.size() && slotIndex < (int)lexicalNames[depth].size()) {
                        varName = lexicalNames[depth][slotIndex];
                    }
                    if (varName.empty() || isAutoGenerated(varName)) continue;

                    // Check if this object is already tracked in heap or pointerTargets
                    int foundId = memory->findObjectId(obj);
                    std::string displayId;
                    if (foundId >= 0) {
                        std::string heapKey = "obj" + std::to_string(foundId);
                        if (memory->getHeap().find(heapKey) != memory->getHeap().end()) {
                            continue; // heap object, already in objectsOnHeap
                        }
                        displayId = "ptr" + std::to_string(foundId);
                    } else {
                        displayId = varName;
                    }

                    stackObjs.push_back(buildObjectViewFromObject(displayId, obj));
                }
            }
            fv.objectsOnStack = std::move(stackObjs);
        }

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
            }

            if (isAutoGenerated(vi.name)) {
                continue;
            }

            // Get type from stored Memory type frame
            std::string storedType = memory->getLexicalVariableType(0, i);
            if (!storedType.empty()) {
                vi.type = storedType;
            } else {
                // Infer type from value
                vi.type = inferValueType(slots[i]);
            }

            vi.value = slots[i].toString();
            fv.variables.push_back(vi);
        }

        // Build stack-local objects for global scope
        {
            std::vector<ObjectView> stackObjs;
            std::set<std::shared_ptr<Object>> seenObjects;
            for (size_t i = 0; i < slots.size(); ++i) {
                const Value& val = slots[i];
                if (val.type != Value::OBJECT_REF || !val.objectRef) continue;

                auto obj = val.objectRef;
                if (seenObjects.find(obj) != seenObjects.end()) continue;
                seenObjects.insert(obj);

                std::string varName;
                int depth = memory->getCurrentLexicalDepth();
                if (depth >= 0 && depth < (int)names.size() && i < names[depth].size()) {
                    varName = names[depth][i];
                }
                if (varName.empty() || isAutoGenerated(varName)) continue;

                int foundId = memory->findObjectId(obj);
                std::string displayId;
                if (foundId >= 0) {
                    std::string heapKey = "obj" + std::to_string(foundId);
                    if (memory->getHeap().find(heapKey) != memory->getHeap().end()) {
                        continue;
                    }
                    displayId = "ptr" + std::to_string(foundId);
                } else {
                    displayId = varName;
                }

                stackObjs.push_back(buildObjectViewFromObject(displayId, obj));
            }
            fv.objectsOnStack = std::move(stackObjs);
        }

        stv.frames.push_back(fv);
    }

    st.stackTrace = stv;

    std::vector<ObjectView> objs;
    for (auto& kv : memory->getHeap()) {
        objs.push_back(buildObjectViewFromObject(kv.first, kv.second));
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
