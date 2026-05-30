#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QVBoxLayout>
#include<string>
#include "qplaintextedit.h"
#include"visualization_data.h"
#include"analyzer.h"
#include<QLabel>
#include"codeeditor.h"
#include <QColor>

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


};

#endif // MAINWINDOW_H
