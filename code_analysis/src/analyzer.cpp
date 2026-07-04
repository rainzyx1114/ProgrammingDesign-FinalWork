#include "analyzer.h"

ASTAnalyzer::ASTAnalyzer(std::shared_ptr<SymbolTable> st,
                         std::shared_ptr<ClassModel> cm)
    : symbolTable(st), classModel(cm), currentClass(nullptr), insideClassMethod(false) {}

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
    // Type resolution is done at runtime (executor); no static type info to resolve here
}

void ASTAnalyzer::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    if (node.index) node.index->accept(*this);
}

void ASTAnalyzer::visit(NewExpr& node) {
    // Verify the class exists in the class model
    if (!classModel->isDefined(node.className)) {
        // Class not yet defined; it might be forward-declared or defined later.
        // For the demo, we allow this and let the executor handle errors.
    }
    for (auto& arg : node.args) {
        if (arg) arg->accept(*this);
    }
}

void ASTAnalyzer::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    // For simple variable assignment, resolve the binding
    if (!node.target && !node.name.lexeme.empty()) {
        Symbol* s = symbolTable->lookup(node.name.lexeme);
        if (s) node.binding = s->binding;
    }
    // For member/array assignment (target is set), visit the l-value
    if (node.target) {
        node.target->accept(*this);
    }
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
    if (currentClass && !insideClassMethod) {
        if (node.type) {
            currentClass->addMember(node.name.lexeme, node.type, node.accessLevel);
        } else {
            currentClass->addMember(node.name.lexeme, Type::createType(TokenType::UNKNOWN), node.accessLevel);
        }
        if (node.initializer) {
            node.initializer->accept(*this);
        }
        return;
    }

    // allocate binding for this declaration and attach it to the VarDecl node
    Binding b = symbolTable->declare(node.name.lexeme, node.type);
    node.binding = b;

    if (node.initializer) {
        node.initializer->accept(*this);
    }
}

void ASTAnalyzer::visit(FuncDecl& node) {
    if (currentClass && !insideClassMethod) {
        // inherit virtualness from overridden base methods
        const ClassDef* baseDef = classModel->getClass(currentClass->baseClass);
        while (baseDef) {
            auto it = baseDef->methods.find(node.name.lexeme);
            if (it != baseDef->methods.end() && it->second.isVirtual) {
                node.isVirtual = true;
                break;
            }
            baseDef = classModel->getClass(baseDef->baseClass);
        }
        currentClass->addMethod(node.name.lexeme, std::make_shared<FuncDecl>(node), node.isVirtual, node.accessLevel);
    }
    // register function
    std::string functionName = node.name.lexeme;
    if (currentClass) {
        functionName = currentClass->name + "::" + functionName;
    }
    symbolTable->declareFunction(functionName, &node);
    // analyze body in its own scope and bind parameters
    symbolTable->enterScope();
    int level = symbolTable->getCurrentLevel();
    node.param_bindings.clear();
    for (auto& p : node.params) {
        Binding b = symbolTable->declare(p.first.lexeme, p.second);
        node.param_bindings.push_back(b);
    }
    node.paramSlotCount = symbolTable->getSlotCountForLevel(level);
    bool previousInside = insideClassMethod;
    if (currentClass) {
        insideClassMethod = true;
    }
    if (node.body) node.body->accept(*this);
    insideClassMethod = previousInside;
    symbolTable->exitScope();
}

void ASTAnalyzer::visit(ClassDecl& node) {
    classModel->defineClass(node.name.lexeme, node.baseClass.lexeme);
    currentClass = classModel->getClass(node.name.lexeme);
    insideClassMethod = false;
    for (auto& member : node.members) {
        if (member) member->accept(*this);
    }
    for (auto& method : node.methods) {
        if (method) method->accept(*this);
    }
    currentClass = nullptr;
    insideClassMethod = false;
}

void ASTAnalyzer::visit(Program& node) {
    for (auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
}

CodeAnalyzer::CodeAnalyzer()
    : isLoaded(false), isExecuting(false), analysisMode(AnalysisMode::MANUAL) {
    memory = std::make_shared<Memory>();
    symbolTable = std::make_shared<SymbolTable>();
    classModel = std::make_shared<ClassModel>();
    astAnalyzer = std::make_shared<ASTAnalyzer>(symbolTable, classModel);
    executor = std::make_shared<Executor>(memory, symbolTable, classModel);
    aiAnalyzer = std::make_shared<AIAnalyzer>();
}

bool CodeAnalyzer::loadCode(const std::string& sourceCode) {
    lastSourceCode = sourceCode;
    lexer = std::make_shared<Lexer>(sourceCode);
    auto tokens = lexer->tokenize();
    for (const auto& token : tokens) {
        if (token.type == TokenType::UNKNOWN) {
            return false;
        }
    }
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

void CodeAnalyzer::start() {
    runContinuously();
}

void CodeAnalyzer::runContinuously() {
    if (!program) return;
    isExecuting = true;
    executor->executeProgram(program);
    isExecuting = false;
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
        // Get base class from class model
        ClassDef* classDef = classModel->getClass(kv.second->className);
        ov.baseClass = classDef ? classDef->baseClass : "";
        for (auto& m : kv.second->members) {
            MemberInfo mi;
            mi.name = m.first;
            mi.type = "";
            mi.value = m.second.toString();
            mi.isMethod = false;
            // Look up access level from class definition
            ClassDef* classDef2 = classModel->getClass(kv.second->className);
            if (classDef2) {
                mi.accessLevel = accessLevelToString(classDef2->getMemberAccess(m.first));
            } else {
                mi.accessLevel = "private";
            }
            ov.members.push_back(mi);
        }
        out.push_back(ov);
    }
    return out;
}

std::vector<ObjectView> CodeAnalyzer::getStackObjects() {
    std::vector<ObjectView> out;
    auto slots = memory->getCurrentLexicalSlots();
    auto names = memory->getLexicalVariableNames();
    int depth = memory->getCurrentLexicalDepth();

    for (size_t i = 0; i < slots.size(); ++i) {
        const Value& val = slots[i];
        if (val.type != Value::OBJECT_REF || !val.objectRef) continue;

        auto obj = val.objectRef;

        // Get variable name
        std::string varName;
        if (depth >= 0 && depth < (int)names.size() && i < names[depth].size()) {
            varName = names[depth][i];
        }
        if (varName.empty()) continue;

        // Skip auto-generated slot names
        if (varName.size() > 4 && varName.find("slot") == 0) {
            bool isAuto = true;
            for (size_t k = 4; k < varName.size(); ++k) {
                if (varName[k] < '0' || varName[k] > '9') { isAuto = false; break; }
            }
            if (isAuto) continue;
        }

        // Determine object ID
        int foundId = memory->findObjectId(obj);
        std::string displayId;
        if (foundId >= 0) {
            std::string heapKey = "obj" + std::to_string(foundId);
            if (memory->getHeap().find(heapKey) != memory->getHeap().end()) {
                continue; // heap object, handled by getObjectsOnHeap()
            }
            displayId = "ptr" + std::to_string(foundId);
        } else {
            displayId = varName;
        }

        ObjectView ov;
        ov.objectId = displayId;
        ov.className = obj->className;
        ClassDef* classDef = classModel->getClass(obj->className);
        ov.baseClass = classDef ? classDef->baseClass : "";
        for (auto& m : obj->members) {
            MemberInfo mi;
            mi.name = m.first;
            mi.type = "";
            mi.value = m.second.toString();
            mi.isMethod = false;
            if (classDef) {
                mi.accessLevel = accessLevelToString(classDef->getMemberAccess(m.first));
            } else {
                mi.accessLevel = "private";
            }
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

std::vector<ClassView> CodeAnalyzer::getAllClassViews() {
    std::vector<ClassView> cvs;
    for (const auto& c : classModel->classes) {
        const ClassDef& classDef = c.second;
        ClassView cv;
        cv.classname = classDef.name;
        cv.baseClass = classDef.baseClass;
        cv.inheritance_depth = 0;
        std::string parent = classDef.baseClass;
        while (!parent.empty()) {
            cv.inheritance_depth++;
            parent = classModel->getBaseClass(parent);
        }

        for (auto& member : classDef.members) {
            MemberInfo mi;
            mi.name = member.name;
            mi.type = member.type ? member.type->toString() : "unknown";
            mi.value = "";
            mi.isMethod = false;
            mi.accessLevel = accessLevelToString(member.accessLevel);
            cv.members.push_back(mi);
        }

        for (auto& methodPair : classDef.methods) {
            MemberInfo mi;
            mi.name = methodPair.first;
            if (methodPair.second.declaration && methodPair.second.declaration->returnType) {
                mi.type = methodPair.second.declaration->returnType->toString();
            } else {
                mi.type = "void";
            }
            mi.value = methodPair.second.isVirtual ? "virtual" : "";
            mi.isMethod = true;
            mi.accessLevel = accessLevelToString(methodPair.second.accessLevel);
            cv.methods.push_back(mi);
        }

        for (const auto& other : classModel->classes) {
            if (other.second.baseClass == classDef.name) {
                cv.derived_classes.push_back(other.second.name);
            }
        }

        const ClassDef* current = &classDef;
        while (current) {
            for (const auto& methodPair : current->methods) {
                if (methodPair.second.isVirtual && cv.vtable.find(methodPair.first) == cv.vtable.end()) {
                    cv.vtable[methodPair.first] = current->name + "::" + methodPair.first;
                }
            }
            if (current->baseClass.empty()) break;
            current = classModel->getClass(current->baseClass);
        }

        cvs.push_back(std::move(cv));
    }
    return cvs;
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

void CodeAnalyzer::setAnalysisMode(AnalysisMode mode) {
    analysisMode = mode;
}

void CodeAnalyzer::setAPIKey(const std::string& key) {
    aiAnalyzer->setAPIKey(key);
}

void CodeAnalyzer::setAPIEndpoint(const std::string& endpoint) {
    aiAnalyzer->setEndpoint(endpoint);
}

void CodeAnalyzer::setAPIModel(const std::string& model) {
    aiAnalyzer->setModel(model);
}

std::shared_ptr<AIAnalysisResult> CodeAnalyzer::getAIResult() {
    if (analysisMode == AnalysisMode::MANUAL) {
        return nullptr;  // AI analysis not requested
    }

    if (!aiAnalyzer->isConfigured()) {
        auto result = std::make_shared<AIAnalysisResult>();
        result->success = false;
        result->errorMessage =
            "AI_TEACHING mode selected but no API key configured. "
            "Use --api-key to provide your key.";
        lastAIResult = result;
        return result;
    }

    if (!isLoaded) {
        auto result = std::make_shared<AIAnalysisResult>();
        result->success = false;
        result->errorMessage = "No code loaded yet. Call loadCode() first.";
        lastAIResult = result;
        return result;
    }

    // Send source code to AI for analysis
    AIAnalysisResult result = aiAnalyzer->analyze(lastSourceCode);

    lastAIResult = std::make_shared<AIAnalysisResult>(std::move(result));
    return lastAIResult;
}

std::shared_ptr<AIAnalysisResult> CodeAnalyzer::askFollowUpQuestion(const std::string& question) {
    if (analysisMode == AnalysisMode::MANUAL) {
        return nullptr;
    }

    if (!aiAnalyzer->isConfigured()) {
        auto result = std::make_shared<AIAnalysisResult>();
        result->success = false;
        result->errorMessage = "API key not configured.";
        return result;
    }

    AIAnalysisResult rawResult = aiAnalyzer->askFollowUp(question);
    lastAIResult = std::make_shared<AIAnalysisResult>(std::move(rawResult));
    return lastAIResult;
}

bool CodeAnalyzer::runFullAnalysis(const std::string& sourceCode) {
    if (!loadCode(sourceCode))
        return false;

    start();  // execute manually first

    // If AI mode is on, trigger AI analysis after execution
    if (analysisMode == AnalysisMode::AI_TEACHING) {
        getAIResult();  // stores result in lastAIResult
    }

    return true;
}
