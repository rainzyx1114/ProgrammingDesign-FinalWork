#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>
#include <QMessageBox>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->toolButton->setDefaultAction(ui->actionfirst);
    ui->toolButton_2->setDefaultAction(ui->actionprev);
    ui->toolButton_3->setDefaultAction(ui->actionnext);
    ui->toolButton_4->setDefaultAction(ui->actionlast);
    QWidget *scrollContent = ui->StackScrollArea->widget();
    if (!scrollContent->layout()) {
        mainStackLayout = new QVBoxLayout(scrollContent);
        scrollContent->setLayout(mainStackLayout);
    }
    else {
        mainStackLayout = qobject_cast<QVBoxLayout*>(scrollContent->layout());
    }

    // 设置布局从上往下排列
    mainStackLayout->setAlignment(Qt::AlignTop);
    //测试
    StackTraceView complexTrace;

    // 1. 最底层的栈帧：main 函数
    StackFrameView mainFrame;
    mainFrame.functionName = "main";
    mainFrame.lineNumber = 105;
    mainFrame.variables.push_back({"argc", "int", "1"});
    mainFrame.variables.push_back({"argv", "char**", "0x7fffc8b3a110"});
    mainFrame.variables.push_back({"dataList", "std::vector<int>", "[42, 15, 88, 7, 99, 23]"}); // 模拟长数组显示
    mainFrame.variables.push_back({"isReady", "bool", "true"});
    complexTrace.frames.push_back(mainFrame);

    // 2. 第二层栈帧：quickSort 函数
    StackFrameView qSortFrame;
    qSortFrame.functionName = "quickSort";
    qSortFrame.lineNumber = 56;
    qSortFrame.variables.push_back({"arr", "std::vector<int>&", "ref -> dataList"});
    qSortFrame.variables.push_back({"low", "int", "0"});
    qSortFrame.variables.push_back({"high", "int", "5"});
    qSortFrame.variables.push_back({"pivotIndex", "int", "2"});
    complexTrace.frames.push_back(qSortFrame);

    // 3. 第三层栈帧：partition 函数 (正在执行循环)
    StackFrameView partitionFrame;
    partitionFrame.functionName = "partition";
    partitionFrame.lineNumber = 34;
    partitionFrame.variables.push_back({"arr", "std::vector<int>&", "ref -> dataList"});
    partitionFrame.variables.push_back({"low", "int", "0"});
    partitionFrame.variables.push_back({"high", "int", "5"});
    partitionFrame.variables.push_back({"pivot", "int", "88"});
    partitionFrame.variables.push_back({"i", "int", "1"});
    partitionFrame.variables.push_back({"j", "int", "3"});
    complexTrace.frames.push_back(partitionFrame);

    // 4. 最顶层的栈帧：swap 函数 (当前程序暂停在这里)
    StackFrameView swapFrame;
    swapFrame.functionName = "swap";
    swapFrame.lineNumber = 12;
    // 模拟传递了引用或指针，并产生了临时变量
    swapFrame.variables.push_back({"a", "int&", "ref -> arr[1] (15)"});
    swapFrame.variables.push_back({"b", "int&", "ref -> arr[3] (7)"});
    swapFrame.variables.push_back({"temp", "int", "15"});
    complexTrace.frames.push_back(swapFrame);

    // 调用你写好的渲染函数！
    renderStackTrace(complexTrace);
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

void MainWindow::clearLayout(QLayout *layout) {
    if (layout == nullptr) return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget(); // 删除控件
        }
        delete item;
    }
}

void MainWindow::renderStackTrace(const StackTraceView& trace)
{
    // 1. 清空旧的界面
    clearLayout(mainStackLayout);

    // 2. 遍历所有的栈帧 (通常调用栈是从底向上，最外层函数在最下面，当前函数在最上面)
    // 为了模仿 Python Tutor，我们采用逆序遍历，把当前正在执行的栈帧放在最上面
    for (auto it = trace.frames.rbegin(); it != trace.frames.rend(); ++it) {
        const StackFrameView& frame = *it;

        // 3. 创建一个 GroupBox 代表这个函数
        QString frameTitle = QString::fromStdString(frame.functionName) +
                             " (Line: " + QString::number(frame.lineNumber) + ")";
        QGroupBox *frameBox = new QGroupBox(frameTitle);
        QVBoxLayout *frameLayout = new QVBoxLayout(frameBox);

        // 4. 创建一个表格来显示变量
        QTableWidget *table = new QTableWidget();
        table->setColumnCount(3); // 3列：名称，类型，值
        table->setHorizontalHeaderLabels(QStringList() << "Name" << "Type" << "Value");

        // 美化表格
        // Name列根据内容自动适应宽度
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        // Type列也根据内容自动适应
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        // Value列最长，让它填满剩余的所有空间！
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false); // 隐藏左侧行号
        table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止双击编辑
        table->setSelectionMode(QAbstractItemView::NoSelection); // 禁止选中

        // 5. 将变量数据填入表格
        int rowCount = frame.variables.size();
        table->setRowCount(rowCount);
        for (int i = 0; i < rowCount; ++i) {
            const VariableInfo& var = frame.variables[i];

            // 注意：队友给的是 std::string，Qt 需要用 QString，所以要转换
            table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(var.name)));
            table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(var.type)));
            table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(var.value)));
        }

        // 根据行数自动调整表格高度，避免出现滚动条
        table->setFixedHeight(table->horizontalHeader()->height() + (table->rowHeight(0) * rowCount) + 2);

        // 6. 把表格装进 GroupBox，把 GroupBox 装进主布局
        frameLayout->addWidget(table);
        mainStackLayout->addWidget(frameBox);
    }
}