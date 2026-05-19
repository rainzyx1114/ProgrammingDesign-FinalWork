#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QVBoxLayout>
#include<vector>
#include<string>

struct VariableInfo {
    std::string name;
    std::string type;
    std::string value;
};

struct StackFrameView {
    std::string functionName;
    int lineNumber;
    std::vector<VariableInfo> variables;
};

struct StackTraceView {
    std::vector<StackFrameView> frames;
};

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

signals:
    void sendCodeToBackend(const std::string& code);
private slots:
    void on_start_button_clicked();


private:
    Ui::MainWindow *ui;
    QVBoxLayout *mainStackLayout; // 用来存放所有栈帧的垂直布局

    void clearLayout(QLayout *layout); // 辅助函数：清空旧UI
};
#endif // MAINWINDOW_H
