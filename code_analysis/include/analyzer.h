#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "memory.h"
#include "symbol_table.h"
#include "executor.h"
#include "class_model.h"
#include "visualization_data.h"
#include "ai_analyzer.h"

// Concrete visitor for analyzing AST
class ASTAnalyzer : public Visitor {
private:
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<ClassModel> classModel;
    ClassDef* currentClass;
    bool insideClassMethod;

public:
    ASTAnalyzer(std::shared_ptr<SymbolTable> st,
                std::shared_ptr<ClassModel> cm);

    void visit(BinaryOp& node) override;
    void visit(LogicalOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(Literal& node) override;
    void visit(Variable& node) override;
    void visit(FunctionCall& node) override;
    void visit(MemberAccess& node) override;
    void visit(ArrayAccess& node) override;
    void visit(NewExpr& node) override;
    void visit(Assignment& node) override;
    void visit(ExprStmt& node) override;
    void visit(Block& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(VarDecl& node) override;
    void visit(FuncDecl& node) override;
    void visit(ClassDecl& node) override;
    void visit(Program& node) override;
};

class CodeAnalyzer {
private:
    std::shared_ptr<Lexer> lexer;
    std::shared_ptr<Parser> parser;
    std::shared_ptr<Memory> memory;
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<ClassModel> classModel;
    std::shared_ptr<Executor> executor;
    std::shared_ptr<ASTAnalyzer> astAnalyzer;

    std::shared_ptr<Program> program;
    bool isLoaded;
    bool isExecuting;

    // AI analysis members
    AnalysisMode analysisMode;
    std::string lastSourceCode;       // stored for AI prompt generation
    std::shared_ptr<AIAnalyzer> aiAnalyzer;
    std::shared_ptr<AIAnalysisResult> lastAIResult;

public:
    CodeAnalyzer();

    // Code loading and parsing
    bool loadCode(const std::string& sourceCode);

    // Execution control
    void start();
    void runContinuously();
    bool isRunning() const { return isExecuting; }

    // State queries
    std::vector<VariableInfo> getVariables();
    std::vector<ObjectView> getObjectsOnHeap();
    std::vector<ObjectView> getStackObjects();
    std::vector<Stepsnapshot> getExecutionTrace();
    std::vector<ClassView> getAllClassViews();

    // Query current position
    int getCurrentLine() const;
    std::string getCurrentFunction() const;

    // --- AI analysis interface ---

    // Set analysis mode: MANUAL (default) or AI_TEACHING
    void setAnalysisMode(AnalysisMode mode);

    // Get current analysis mode
    AnalysisMode getAnalysisMode() const { return analysisMode; }

    // Configure AI analysis with user's API key, optional endpoint and model
    void setAPIKey(const std::string& key);
    void setAPIEndpoint(const std::string& endpoint);
    void setAPIModel(const std::string& model);

    // After loadCode() + start() (manual execution), call this to get
    // an AI-powered teaching explanation.  Returns nullptr in MANUAL mode
    // or if something went wrong.
    std::shared_ptr<AIAnalysisResult> getAIResult();

    // Ask a follow-up question about the previously analysed code.
    // Returns nullptr in MANUAL mode or if no analysis has been performed yet.
    std::shared_ptr<AIAnalysisResult> askFollowUpQuestion(const std::string& question);

    // Convenience: load, execute, and optionally AI-analyse in one call
    // Returns true if code loaded successfully (execution + AI always run if configured)
    bool runFullAnalysis(const std::string& sourceCode);
};

#endif
