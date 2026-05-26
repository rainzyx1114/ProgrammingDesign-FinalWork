#include "analyzer.h"

ASTAnalyzer::ASTAnalyzer(std::shared_ptr<SymbolTable> st, std::shared_ptr<TypeSystem> ts,
                         std::shared_ptr<ClassModel> cm)
    : symbolTable(st), typeSystem(ts), classModel(cm) {}

void ASTAnalyzer::visit(BinaryOp& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void ASTAnalyzer::visit(LogicalOp& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void ASTAnalyzer::visit(UnaryOp& node) {
    if (node.operand) node.operand->accept(*this);
}

void ASTAnalyzer::visit(Literal& node) {
}

void ASTAnalyzer::visit(Variable& node) {
    Symbol* s = symbolTable->lookup(node.name.lexeme);
    if (s) {
        node.binding = s->binding;
    }
}

void ASTAnalyzer::visit(FunctionCall& node) {
    if (node.name) node.name->accept(*this);
    for (auto& arg : node.args) {
        if (arg) arg->accept(*this);
    }
}

void ASTAnalyzer::visit(MemberAccess& node) {
    if (node.object) node.object->accept(*this);
}

void ASTAnalyzer::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    if (node.index) node.index->accept(*this);
}

void ASTAnalyzer::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    Symbol* s = symbolTable->lookup(node.name.lexeme);
    if (s) node.binding = s->binding;
    // Check assignment compatibility
}

void ASTAnalyzer::visit(ExprStmt& node) {
    if (node.expr) node.expr->accept(*this);
}

void ASTAnalyzer::visit(Block& node) {
    symbolTable->enterScope();
    int level = symbolTable->getCurrentLevel();
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    node.slotCount = symbolTable->getSlotCountForLevel(level);
    symbolTable->exitScope();
}

void ASTAnalyzer::visit(IfStmt& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.thenBranch) node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void ASTAnalyzer::visit(WhileStmt& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void ASTAnalyzer::visit(ForStmt& node) {
    symbolTable->enterScope();
    if (node.init) node.init->accept(*this);
    if (node.condition) node.condition->accept(*this);
    if (node.update) node.update->accept(*this);
    if (node.body) node.body->accept(*this);
    symbolTable->exitScope();
}

void ASTAnalyzer::visit(ReturnStmt& node) {
    if (node.value) node.value->accept(*this);
}

void ASTAnalyzer::visit(VarDecl& node) {
    // allocate binding for this declaration and attach it to the VarDecl node
    Binding b = symbolTable->declare(node.name.lexeme, node.type, node.name);
    node.binding = b;

    if (node.initializer) {
        node.initializer->accept(*this);
        symbolTable->markInitialized(node.name.lexeme);
    }
}

void ASTAnalyzer::visit(FuncDecl& node) {
    // register function 
    symbolTable->declareFunction(node.name.lexeme, &node);
    // analyze body in its own scope and bind parameters
    symbolTable->enterScope();
    int level = symbolTable->getCurrentLevel();
    node.param_bindings.clear();
    for (auto& p : node.params) {
        Binding b = symbolTable->declare(p.first.lexeme, p.second, p.first);
        node.param_bindings.push_back(b);
        symbolTable->markInitialized(p.first.lexeme);
    }
    node.paramSlotCount = symbolTable->getSlotCountForLevel(level);
    if (node.body) node.body->accept(*this);
    symbolTable->exitScope();
}

void ASTAnalyzer::visit(ClassDecl& node) {
    classModel->defineClass(node.name.lexeme, node.baseClass.lexeme);
    for (auto& member : node.members) {
        if (member) member->accept(*this);
    }
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
}

void ASTAnalyzer::visit(Program& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

CodeAnalyzer::CodeAnalyzer()
    : isLoaded(false), isExecuting(false) {
    memory = std::make_shared<Memory>();
    symbolTable = std::make_shared<SymbolTable>();
    typeSystem = std::make_shared<TypeSystem>();
    classModel = std::make_shared<ClassModel>();
    astAnalyzer = std::make_shared<ASTAnalyzer>(symbolTable, typeSystem, classModel);
    executor = std::make_shared<Executor>(memory, symbolTable, typeSystem, classModel);
}

bool CodeAnalyzer::loadCode(const std::string& sourceCode) {
    lexer = std::make_shared<Lexer>(sourceCode);
    auto tokens = lexer->tokenize();
    parser = std::make_shared<Parser>(tokens);
    program = parser->parse();
    
    if (program) {
        // Use visitor to analyze the AST
        program->accept(*astAnalyzer);
        isLoaded = true;
        return true;
    }
    return false;
}

std::string CodeAnalyzer::getParseError() const {
    // Implementation
    return "";
}

void CodeAnalyzer::start() {
    runContinuously();
}

// void CodeAnalyzer::stepExecute() {
//     
// }

void CodeAnalyzer::runContinuously() {
    if (!program) return;
    isExecuting = true;
    executor->executeProgram(program);
    isExecuting = false;
}

// void CodeAnalyzer::pause() {
//    
// }

// void CodeAnalyzer::stop() {
//    
// }

ExecutionState CodeAnalyzer::getExecutionState() {
    ExecutionState st;
    st.currentLine = getCurrentLine();
    st.currentFunction = getCurrentFunction();
    st.stackTrace = getStackTrace();
    st.objectsOnHeap = getObjectsOnHeap();
    return st;
}

StackTraceView CodeAnalyzer::getStackTrace() {
    StackTraceView stv;
    const auto& callStack = memory->getCallStack();
    for (int frameIndex = 0; frameIndex < (int)callStack.size(); ++frameIndex) {
        auto frame = callStack[frameIndex];
        StackFrameView sfv;
        sfv.functionName = frame->functionName;
        sfv.lineNumber = frame->lineNumber;
        auto lexicalFrames = memory->getLexicalFramesForCallFrame(frameIndex);
        auto lexicalNames = memory->getLexicalVariableNamesForCallFrame(frameIndex);
        int callerDepth = frame->lexicalVariableNames.size();
        if (callerDepth > 0 && callerDepth < (int)lexicalFrames.size()) {
            lexicalFrames = std::vector<std::vector<Value>>(lexicalFrames.begin() + callerDepth, lexicalFrames.end());
        }
        if (callerDepth > 0 && callerDepth < (int)lexicalNames.size()) {
            lexicalNames = std::vector<std::vector<std::string>>(lexicalNames.begin() + callerDepth, lexicalNames.end());
        }
        for (int depth = 0; depth < (int)lexicalFrames.size(); ++depth) {
            for (int slot = 0; slot < (int)lexicalFrames[depth].size(); ++slot) {
                VariableInfo vi;
                if (depth < (int)lexicalNames.size() && slot < (int)lexicalNames[depth].size()) {
                    vi.name = lexicalNames[depth][slot];
                } else {
                    vi.name = "slot" + std::to_string(slot);
                }
                vi.type = "";
                vi.value = lexicalFrames[depth][slot].toString();
                sfv.variables.push_back(vi);
            }
        }
        stv.frames.push_back(sfv);
    }
    return stv;
}

std::vector<VariableInfo> CodeAnalyzer::getVariables() {
    std::vector<VariableInfo> out;
    auto slots = memory->getCurrentLexicalSlots();
    auto names = memory->getLexicalVariableNames();
    int depth = memory->getCurrentLexicalDepth();
    for (size_t i = 0; i < slots.size(); ++i) {
        VariableInfo vi;
        if (depth >= 0 && depth < (int)names.size() && i < names[depth].size()) {
            vi.name = names[depth][i];
        } else {
            vi.name = "slot" + std::to_string(i);
        }
        
        // Get type from symbol table if available
        Symbol* sym = symbolTable->lookupByBinding(depth, i);
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
        out.push_back(vi);
    }
    return out;
}

std::vector<ObjectView> CodeAnalyzer::getObjectsOnHeap() {
    std::vector<ObjectView> out;
    for (auto& kv : memory->getHeap()) {
        ObjectView ov;
        ov.objectId = kv.first;
        ov.className = kv.second->className;
        ov.baseClass = "";
        for (auto& m : kv.second->members) {
            MemberInfo mi;
            mi.name = m.first;
            mi.type = "";
            mi.value = m.second.toString();
            mi.isMethod = false;
            ov.members.push_back(mi);
        }
        out.push_back(ov);
    }
    return out;
}

std::vector<Stepsnapshot> CodeAnalyzer::getExecutionTrace() {
    if (executor) {
        return executor->getExecutionTrace();
    }
    return {};
}

int CodeAnalyzer::getCurrentLine() const {
    auto frame = memory->currentFrame();
    if (!frame) return 0;
    return frame->lineNumber;
}

std::string CodeAnalyzer::getCurrentFunction() const {
    auto frame = memory->currentFrame();
    if (!frame) return std::string();
    return frame->functionName;
}
