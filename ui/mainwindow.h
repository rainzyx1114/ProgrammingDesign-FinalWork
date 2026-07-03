#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QVBoxLayout>
#include<string>
#include "qplaintextedit.h"
#include"visualization_data.h"
#include"analyzer.h"
#include"ai_analyzer.h"
#include<QLabel>
#include"codeeditor.h"
#include <QColor>
#include "knowledgebookwidget.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QSplitter>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void renderStackTrace(const StackTraceView& trace);
    void renderEventInfo(const std::string& event, int currentLine, const std::string& currentFunction);
    void renderHeapObjects(const std::vector<ObjectView>& objects);
    void renderClassViews(const std::vector<ClassView>& classViews);

signals:
    void sendCodeToBackend(const std::string& code);
private slots:
    void on_start_button_clicked();
    void onStepChanged(int step);
    void on_actionnext_triggered();
    void on_actionprev_triggered();
    void on_actionfirst_triggered();
    void on_actionlast_triggered();
    void on_actionFontSize_triggered();
    void on_actionFontColor_triggered();
    void on_actionUserGuide_triggered();
    void on_actionKnowledgeBook_triggered();
    void on_actionAPISettings_triggered();
    void on_aiSendButton_clicked();
    void on_aiToggleButton_clicked();
    void on_aiStopButton_clicked();
    void on_aiModeButton_clicked();


private:
    Ui::MainWindow *ui;
    std::shared_ptr<CodeAnalyzer> m_analyzer;
    QVBoxLayout *mainStackLayout; // 用来存放所有栈帧的垂直布局
    QVBoxLayout* mainHeapLayout;
    QVBoxLayout* mainClassLayout;

    void clearLayout(QLayout *layout); // 辅助函数：清空旧UI
    std::vector<Stepsnapshot>m_trace;
    int m_currentStep=-1;
    QLabel*m_stepLabel=nullptr;
    CodeEditor* m_codeEditor;
    QLabel* m_eventLabel;
    QLabel* m_functionLabel;

    void updateExecutionArrows();
    bool eventFilter(QObject *obj, QEvent *event) override;

    // ========== AI 侧边栏 ==========
    void createAISidebar();
    void appendChatMessage(const QString &sender, const QString &htmlContent);
    QString formatAIResponse(const AIAnalysisResult &result);
    QString renderAIResponse(const AIAnalysisResult &result);
    void collapseAISidebar();
    void expandAISidebar();
    void startAIAnalysis();   // 代码执行后自动触发首次 AI 分析
    void startAIAnalysisDirect(const std::string &sourceCode);  // 绕过 loadCode 失败限制，直接调用 AI

    // ========== AI 侧边栏控件 ==========
    QWidget *m_aiSidebar = nullptr;
    QWidget *m_aiHeaderArea = nullptr;
    QWidget *m_aiContentArea = nullptr;
    QTextBrowser *m_aiChatDisplay = nullptr;
    QTextEdit *m_aiInputEdit = nullptr;
    QPushButton *m_aiSendButton = nullptr;
    QPushButton *m_aiStopButton = nullptr;
    QPushButton *m_aiToggleButton = nullptr;
    QPushButton *m_aiModeButton = nullptr;
    QLabel *m_aiLoadingLabel = nullptr;
    QLabel *m_aiModeLabel = nullptr;
    QLabel *m_aiWelcomeLabel = nullptr;
    QSplitter *m_aiInputSplitter = nullptr;
    bool m_aiSidebarExpanded = true;
    QList<int> m_savedSplitterSizes;

    // ========== AI 配置（会话期间存储）==========
    std::string m_aiApiKey;
    std::string m_aiApiEndpoint = "https://api.deepseek.com/v1/chat/completions";
    std::string m_aiApiModel = "deepseek-chat";
    AnalysisMode m_aiMode = AnalysisMode::AI_TEACHING;
    bool m_aiRequestInFlight = false;
    bool m_aiFirstAnalysisDone = false;

    // ========== 对话历史 HTML ==========
    QString m_aiChatHtml;

    // ========== 活跃的异步任务 ==========
    QFutureWatcher<AIAnalysisResult> *m_aiWatcher = nullptr;

    QSplitter *m_mainSplitter = nullptr;
};

#endif // MAINWINDOW_H

Q_DECLARE_METATYPE(AIAnalysisResult)
