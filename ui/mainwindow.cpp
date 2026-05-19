#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>
#include <QMessageBox>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->toolButton->setDefaultAction(ui->actionfirst);
    ui->toolButton_2->setDefaultAction(ui->actionprev);
    ui->toolButton_3->setDefaultAction(ui->actionnext);
    ui->toolButton_4->setDefaultAction(ui->actionlast);
}
void MainWindow::on_start_button_clicked() // 假设你的按钮叫 pushButton
{
    // 1. 读取代码框中的所有文字
    QString qcodeText = ui->plainTextEdit->toPlainText();

    // 2. 如果代码为空，给个提示（可选）
    if(qcodeText.trimmed().isEmpty()){
        QMessageBox::warning(this,"警告","代码不能为空");
        return;
    }
    std::string codeText=qcodeText.toStdString();
    // 3. 把代码通过信号发射出去，交给队友的类去处理
    qDebug()<<codeText;
    emit sendCodeToBackend(codeText);

    // 4. UI 上的状态改变（比如把“开始”按钮变灰，防止重复点击）
    ui->start_button->setEnabled(false);
}
MainWindow::~MainWindow()
{
    delete ui;
}
