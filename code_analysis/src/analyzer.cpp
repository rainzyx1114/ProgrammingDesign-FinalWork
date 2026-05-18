#include "analyzer.h"

ASTAnalyzer::ASTAnalyzer(std::shared_ptr<SymbolTable> st, std::shared_ptr<TypeSystem> ts,
                         std::shared_ptr<ClassModel> cm, std::shared_ptr<Memory> mem)
    : symbolTable(st), typeSystem(ts), classModel(cm), memory(mem) {}

void ASTAnalyzer::visit(BinaryOp& node) {
    // Analyze left and right operands
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
    // Perform type checking for binary operations
    // Implementation depends on type system
}

void ASTAnalyzer::visit(LogicalOp& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
    // Type checking for logical operations
}

void ASTAnalyzer::visit(UnaryOp& node) {
    if (node.operand) node.operand->accept(*this);
    // Type checking for unary operations
}

void ASTAnalyzer::visit(Literal& node) {
    // Handle literal values
}

void ASTAnalyzer::visit(Variable& node) {
    // Check if variable is declared in symbol table
    // symbolTable->lookup(node.name.lexeme);
}

void ASTAnalyzer::visit(FunctionCall& node) {
    if (node.name) node.name->accept(*this);
    for (auto& arg : node.args) {
        if (arg) arg->accept(*this);
    }
    // Check function signature and arguments
}

void ASTAnalyzer::visit(MemberAccess& node) {
    if (node.object) node.object->accept(*this);
    // Check member access validity
}

void ASTAnalyzer::visit(ArrayAccess& node) {
    if (node.array) node.array->accept(*this);
    if (node.index) node.index->accept(*this);
    // Check array bounds and type
}

void ASTAnalyzer::visit(Assignment& node) {
    if (node.value) node.value->accept(*this);
    // Check assignment compatibility
}

void ASTAnalyzer::visit(ExprStmt& node) {
    if (node.expr) node.expr->accept(*this);
}

void ASTAnalyzer::visit(Block& node) {
    // Enter new scope
    symbolTable->enterScope();
    for (auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
    // Exit scope
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
    // Check return type compatibility
}

void ASTAnalyzer::visit(VarDecl& node) {
    // Add variable to symbol table
    // symbolTable->declare(node.name.lexeme, node.type);
    if (node.initializer) node.initializer->accept(*this);
}

void ASTAnalyzer::visit(FuncDecl& node) {
    // Add function to symbol table
    // symbolTable->declareFunction(node.name.lexeme, node.returnType, node.params);
    if (node.body) node.body->accept(*this);
}

void ASTAnalyzer::visit(ClassDecl& node) {
    // Add class to class model
    // classModel->addClass(node.name.lexeme, node.baseClass.lexeme);
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
    astAnalyzer = std::make_shared<ASTAnalyzer>(symbolTable, typeSystem, classModel, memory);
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
    // Implementation
}

void CodeAnalyzer::stepExecute() {
    // Implementation
}

void CodeAnalyzer::runContinuously() {
    // Implementation
}

void CodeAnalyzer::pause() {
    // Implementation
}

void CodeAnalyzer::stop() {
    // Implementation
}

ExecutionState CodeAnalyzer::getExecutionState() {
    // Implementation
    return ExecutionState();
}

StackTraceView CodeAnalyzer::getStackTrace() {
    // Implementation
    return StackTraceView();
}

std::vector<VariableInfo> CodeAnalyzer::getVariables() {
    // Implementation
    return std::vector<VariableInfo>();
}

std::vector<ObjectView> CodeAnalyzer::getObjectsOnHeap() {
    // Implementation
    return std::vector<ObjectView>();
}

int CodeAnalyzer::getCurrentLine() const {
    // Implementation
    return 0;
}

std::string CodeAnalyzer::getCurrentFunction() const {
    // Implementation
    return "";
}
