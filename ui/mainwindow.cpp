#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "codeeditor.h"
#include<QDebug>
#include <QMessageBox>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QScrollArea>
#include <QFrame>
#include <QInputDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QTextCharFormat>
#include <QTextBrowser>
#include <QPushButton>

// Helper: 将 event 字符串转换为用户友好的描述
static QString eventToDisplayString(const std::string& event) {
    if (event == "var_decl")       return QString::fromUtf8("📝 变量声明");
    if (event == "assignment")     return QString::fromUtf8("✏️ 赋值操作");
    if (event == "call_enter")     return QString::fromUtf8("📞 进入函数调用");
    if (event == "call_return")    return QString::fromUtf8("↩️ 函数返回");
    if (event == "return")         return QString::fromUtf8("↩️ return 语句");
    if (event == "scope_exit")     return QString::fromUtf8("📤 退出作用域");
    if (event == "program_end")    return QString::fromUtf8("🏁 程序结束");
    return QString::fromStdString(event);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	m_analyzer=std::make_shared<CodeAnalyzer>();
    ui->toolButton->setDefaultAction(ui->actionfirst);
    ui->toolButton_2->setDefaultAction(ui->actionprev);
    ui->toolButton_3->setDefaultAction(ui->actionnext);
    ui->toolButton_4->setDefaultAction(ui->actionlast);

    // 限制四个步进按钮的尺寸，防止初始时过大
    QList<QToolButton*> stepButtons = {ui->toolButton, ui->toolButton_2, ui->toolButton_3, ui->toolButton_4};
    for (auto *btn : stepButtons) {
        btn->setFixedSize(100, 60);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    // 限制开始按钮的尺寸
    ui->start_button->setMinimumSize(100, 60);
    ui->start_button->setMaximumSize(120, 80);
    ui->start_button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 用 CodeEditor 替换 UI 中的 plainTextEdit，保持与 .ui 文件兼容
    {
        QPlainTextEdit *oldEdit = ui->plainTextEdit;
        // 获取旧编辑器所在的 splitter
        QSplitter *splitter = qobject_cast<QSplitter*>(oldEdit->parentWidget());

        if (splitter) {
            // 保存旧编辑器的属性
            QFont oldFont = oldEdit->font();
            int indexInSplitter = splitter->indexOf(oldEdit);
            QList<int> sizes = splitter->sizes();

            // 创建新的 CodeEditor
            m_codeEditor = new CodeEditor(splitter);
            m_codeEditor->setFont(oldFont);
            m_codeEditor->setPlainText(oldEdit->toPlainText());

            // 在 splitter 中替换：先插入新的，再删除旧的
            splitter->insertWidget(indexInSplitter, m_codeEditor);
            delete oldEdit;

            // 恢复 splitter 的大小比例
            splitter->setSizes(sizes);
        } else {
            // 如果不在 splitter 中，回退方案
            QWidget *parent = oldEdit->parentWidget();
            m_codeEditor = new CodeEditor(parent);
            m_codeEditor->setFont(oldEdit->font());
            m_codeEditor->setPlainText(oldEdit->toPlainText());
            delete oldEdit;
        }

        // 让 ui->plainTextEdit 指向新的 CodeEditor（通过对象名关联）
        m_codeEditor->setObjectName("plainTextEdit");
    }

    // 设置 splitter_2 的初始大小比例：代码编辑器占 75%，底部控件占 25%
    ui->splitter_2->setSizes(QList<int>() << 600 << 200);

    // ========== 动态重建右侧面板 ==========
    // 找到主 splitter（水平方向，左边是代码区，右边是 StackScrollArea）
    QSplitter *mainSplitter = qobject_cast<QSplitter*>(ui->StackScrollArea->parentWidget());
    int scrollIndex = -1;
    QList<int> mainSizes;
    if (mainSplitter) {
        scrollIndex = mainSplitter->indexOf(ui->StackScrollArea);
        mainSizes = mainSplitter->sizes();
    }

    // 创建新的右侧滚动区域
    QScrollArea *rightScrollArea = new QScrollArea();
    rightScrollArea->setWidgetResizable(true);
    rightScrollArea->setMinimumWidth(380);
    rightScrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background-color: #F3F4F6;
        }
        #qt_scrollarea_viewport, #qt_scrollarea_widgetcontents {
            background-color: #F3F4F6;
        }
        QGroupBox {
            background-color: #FFFFFF;
            border: 1px solid #E5E7EB;
            border-radius: 8px;
            margin-top: 24px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 15px;
            top: 5px;
            color: #2563EB;
            font-weight: bold;
            font-size: 14px;
            background-color: transparent;
        }
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: #E5E7EB;
            color: #1F2937;
        }
        QHeaderView::section {
            background-color: #F9FAFB;
            color: #6B7280;
            border: none;
            border-bottom: 2px solid #E5E7EB;
            font-weight: bold;
            padding: 4px;
        }
        QScrollBar:vertical {
            border: none;
            background: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #D1D5DB;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9CA3AF;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    // 右侧面板容器
    QWidget *rightPanel = new QWidget();
    rightPanel->setStyleSheet("background-color: #F3F4F6;");
    QVBoxLayout *rightPanelLayout = new QVBoxLayout(rightPanel);
    rightPanelLayout->setSpacing(8);
    rightPanelLayout->setContentsMargins(8, 8, 8, 8);

    // --- 1. 事件信息 + 当前函数 行 ---
    QWidget *infoBar = new QWidget();
    QHBoxLayout *infoBarLayout = new QHBoxLayout(infoBar);
    infoBarLayout->setContentsMargins(0, 0, 0, 0);
    infoBarLayout->setSpacing(8);

    m_eventLabel = new QLabel(QString::fromUtf8("Event: -"));
    m_eventLabel->setObjectName("eventLabel");
    m_eventLabel->setStyleSheet(R"(
        background-color: #FEF3C7;
        border: 1px solid #F59E0B;
        border-radius: 6px;
        padding: 6px 12px;
        color: #92400E;
        font-weight: bold;
        font-size: 13px;
    )");

    m_functionLabel = new QLabel(QString::fromUtf8("Function: -"));
    m_functionLabel->setObjectName("functionLabel");
    m_functionLabel->setStyleSheet(R"(
        background-color: #DBEAFE;
        border: 1px solid #3B82F6;
        border-radius: 6px;
        padding: 6px 12px;
        color: #1E40AF;
        font-weight: bold;
        font-size: 13px;
    )");

    infoBarLayout->addWidget(m_eventLabel);
    infoBarLayout->addWidget(m_functionLabel);
    infoBarLayout->addStretch();
    rightPanelLayout->addWidget(infoBar);

    // --- 2. Stack Frames 分区 ---
    QLabel *stackTitle = new QLabel(QString::fromUtf8("📋 Stack Frames"));
    stackTitle->setStyleSheet("color: #374151; font-size: 15px; font-weight: bold; padding: 4px 0px;");
    rightPanelLayout->addWidget(stackTitle);

    QWidget *stackContainer = new QWidget();
    mainStackLayout = new QVBoxLayout(stackContainer);
    mainStackLayout->setSpacing(4);
    mainStackLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->addWidget(stackContainer);

    // --- 3. Heap Objects 分区 ---
    QLabel *heapTitle = new QLabel(QString::fromUtf8("🗂️ Heap Objects"));
    heapTitle->setStyleSheet("color: #374151; font-size: 15px; font-weight: bold; padding: 4px 0px;");
    rightPanelLayout->addWidget(heapTitle);

    QWidget *heapContainer = new QWidget();
    mainHeapLayout = new QVBoxLayout(heapContainer);
    mainHeapLayout->setSpacing(4);
    mainHeapLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->addWidget(heapContainer);

    // --- 4. Class Definitions 分区 ---
    QLabel *classTitle = new QLabel(QString::fromUtf8("🏗️ Class Definitions"));
    classTitle->setStyleSheet("color: #374151; font-size: 15px; font-weight: bold; padding: 4px 0px;");
    rightPanelLayout->addWidget(classTitle);

    QWidget *classContainer = new QWidget();
    mainClassLayout = new QVBoxLayout(classContainer);
    mainClassLayout->setSpacing(4);
    mainClassLayout->setContentsMargins(0, 0, 0, 0);
    rightPanelLayout->addWidget(classContainer);

    // 底部弹簧
    rightPanelLayout->addStretch();

    rightScrollArea->setWidget(rightPanel);

    // 替换主 splitter 中的旧 StackScrollArea
    if (mainSplitter && scrollIndex >= 0) {
        mainSplitter->insertWidget(scrollIndex, rightScrollArea);
        delete ui->StackScrollArea;
        ui->StackScrollArea = nullptr;
        mainSplitter->setSizes(mainSizes);
    }

	ui->slider->setMaximum(0);
	ui->slider->setValue(0);
    connect(ui->slider,&QSlider::valueChanged,this,&MainWindow::onStepChanged);

    // 设置主水平 splitter 的初始大小比例：左侧代码区 60%，右侧可视化区 40%
    QSplitter *horizontalSplitter = qobject_cast<QSplitter*>(ui->left_widget->parentWidget());
    if (horizontalSplitter) {
        horizontalSplitter->setSizes(QList<int>() << 768 << 512);
    }
	QMenu *knowledgeBookMenu = ui->menubar->addMenu(QString::fromUtf8("知识手册"));
    QAction *actionKnowledgeBook = new QAction(QString::fromUtf8("打开知识手册"), this);
    actionKnowledgeBook->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    knowledgeBookMenu->addAction(actionKnowledgeBook);
    connect(actionKnowledgeBook, &QAction::triggered, this, &MainWindow::on_actionKnowledgeBook_triggered);
}
void MainWindow::on_start_button_clicked() 
{
    // 1. 读取代码框中的所有文字
    QString qcodeText = m_codeEditor->toPlainText();

    // 2. 如果代码为空，给个提示
    if(qcodeText.trimmed().isEmpty()){
        QMessageBox::warning(this,"警告","代码不能为空");
        return;
    }
    // 3. 重建 CodeAnalyzer，彻底重置内部状态（Memory、Executor、SymbolTable 等）
    //    避免旧的执行轨迹残留导致 step 越来越多
    m_analyzer = std::make_shared<CodeAnalyzer>();

    std::string codeText=qcodeText.toStdString();
    // 4. 加载并执行代码
	if(!m_analyzer->loadCode(codeText)){
		qDebug()<<"load code failed";
		return;
	}
	m_analyzer->start();
	m_trace=m_analyzer->getExecutionTrace();
	int totalSteps=m_trace.size();
	if (!m_stepLabel){
		m_stepLabel=new QLabel(QString("Step: 0/%1").arg(totalSteps-1>0?totalSteps-1:0),this);
		QLayout*parentLayout= ui->slider->parentWidget()->layout();
		if (parentLayout){
            parentLayout->addWidget(m_stepLabel);
		}
	} else {
		m_stepLabel->setText(QString("Step: 0/%1").arg(totalSteps-1>0?totalSteps-1:0));
	}
	ui->slider->setMaximum(totalSteps-1?totalSteps-1:0);
	ui->slider->setValue(0);
    // 4. UI 上的状态改变（比如把“开始”按钮变灰，防止重复点击）
    ui->start_button->setText(QString::fromUtf8("开始"));
    // 重置当前步数，确保 onStepChanged(0) 能正确触发
    m_currentStep = -1;
    // 清除旧的执行箭头和高亮
    m_codeEditor->clearExecutionLines();
    // 重置右侧可视化面板
    clearLayout(mainStackLayout);
    clearLayout(mainHeapLayout);
    clearLayout(mainClassLayout);
    if (m_eventLabel) m_eventLabel->setText(QString::fromUtf8("Event: -"));
    if (m_functionLabel) m_functionLabel->setText(QString::fromUtf8("Function: -"));
    // 显示第一步
    if (!m_trace.empty()) {
        onStepChanged(0);
    }
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStepChanged(int step){
	if(m_trace.empty()||step<0||step>=m_trace.size()){
		return;
	}
	m_currentStep=step;
	ui->slider->blockSignals(true);
	ui->slider->setValue(step);
	ui->slider->blockSignals(false);
	if(m_stepLabel){
		m_stepLabel->setText(QString("Step: %1/%2").arg(step).arg(m_trace.size()-1>0?m_trace.size()-1:0));
	}
	const Stepsnapshot& snapshot=m_trace[step];
	qDebug()<<snapshot.event;
    for (const auto& frame:snapshot.state.stackTrace.frames){
        qDebug()<<frame.functionName;
        for(const auto & var:frame.variables){
            qDebug()<<var.name;
            qDebug()<<var.type;
        }
    }
    qDebug();
    // 渲染所有可视化面板
    renderEventInfo(snapshot.event, snapshot.state.currentLine, snapshot.state.currentFunction);
    renderStackTrace(snapshot.state.stackTrace);
    renderHeapObjects(snapshot.state.objectsOnHeap);
    renderClassViews(m_analyzer->getAllClassViews());
    updateExecutionArrows();
}

void MainWindow::on_actionfirst_triggered()
{
	onStepChanged(0);
}
void MainWindow::on_actionprev_triggered(){
    if(m_currentStep<1){
        QMessageBox::warning(this,"警告","已经是第一步了");
    }
	onStepChanged(m_currentStep-1);
}
void MainWindow::on_actionnext_triggered(){
    if(m_currentStep+1>=m_trace.size()){
        QMessageBox::warning(this,"警告","已经是最后一步了");
    }
	onStepChanged(m_currentStep+1);
}
void MainWindow::on_actionlast_triggered(){
	onStepChanged(m_trace.size()-1);
}
void MainWindow::updateExecutionArrows()
{
    if (!m_codeEditor || m_trace.empty() || m_currentStep < 0 || m_currentStep >= m_trace.size()) {
        if (m_codeEditor) m_codeEditor->clearExecutionLines();
        return;
    }

    // 当前步骤的执行行
    int currentLine = m_trace[m_currentStep].state.currentLine;

    // 下一条要执行的行（下一步的执行行）
    int nextLine = -1;
    if (m_currentStep + 1 < m_trace.size()) {
        nextLine = m_trace[m_currentStep + 1].state.currentLine;
    }

    m_codeEditor->setCurrentLine(currentLine);
    m_codeEditor->setNextLine(nextLine);
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

void MainWindow::renderEventInfo(const std::string& event, int currentLine, const std::string& currentFunction)
{
    if (m_eventLabel) {
        QString eventText = eventToDisplayString(event);
        if (currentLine > 0) {
            m_eventLabel->setText(QString::fromUtf8("Event: %1  (Line %2)")
                .arg(eventText)
                .arg(currentLine));
        } else {
            m_eventLabel->setText(QString::fromUtf8("Event: %1").arg(eventText));
        }
    }
    if (m_functionLabel) {
        QString funcName = currentFunction.empty()
            ? QString::fromUtf8("全局作用域")
            : QString::fromStdString(currentFunction);
        m_functionLabel->setText(QString::fromUtf8("Function: %1").arg(funcName));
    }
}

void MainWindow::renderHeapObjects(const std::vector<ObjectView>& objects)
{
    clearLayout(mainHeapLayout);

    if (objects.empty()) {
        QLabel *emptyLabel = new QLabel(QString::fromUtf8("  (堆上暂无对象)"));
        emptyLabel->setStyleSheet("color: #9CA3AF; font-style: italic; padding: 4px;");
        mainHeapLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto& obj : objects) {
        // 每个堆对象一个 GroupBox，橙色风格（Python Tutor 的 heap 区域风格）
        QString title = QString::fromStdString(obj.className) +
                        " [" + QString::fromStdString(obj.objectId) + "]";
        QGroupBox *objBox = new QGroupBox(title);
        objBox->setStyleSheet(R"(
            QGroupBox {
                background-color: #FFF7ED;
                border: 1px solid #FDBA74;
                border-radius: 8px;
                margin-top: 24px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 15px;
                top: 5px;
                color: #C2410C;
                font-weight: bold;
                font-size: 13px;
                background-color: transparent;
            }
        )");

        QVBoxLayout *objLayout = new QVBoxLayout(objBox);

        // 如果有基类信息，显示继承关系
        if (!obj.baseClass.empty()) {
            QLabel *baseLabel = new QLabel(
                QString::fromUtf8("extends ") + QString::fromStdString(obj.baseClass));
            baseLabel->setStyleSheet("color: #92400E; font-size: 12px; padding: 2px 8px;");
            objLayout->addWidget(baseLabel);
        }

        // 成员变量表格
        if (!obj.members.empty()) {
            QTableWidget *table = new QTableWidget();
            table->setColumnCount(3);
            table->setHorizontalHeaderLabels(
                QStringList() << QString::fromUtf8("成员") << QString::fromUtf8("类型") << QString::fromUtf8("值"));

            table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
            table->verticalHeader()->setVisible(false);
            table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            table->setSelectionMode(QAbstractItemView::NoSelection);

            int row = 0;
            for (const auto& mem : obj.members) {
                if (mem.isMethod) continue; // 跳过方法，只显示数据成员
                table->setRowCount(row + 1);
                table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(mem.name)));
                table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(mem.type)));
                table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(mem.value)));
                ++row;
            }

            // 如果没有数据成员（全是方法），显示提示
            if (row == 0) {
                delete table;
                QLabel *noDataLabel = new QLabel(QString::fromUtf8("  (无数据成员)"));
                noDataLabel->setStyleSheet("color: #9CA3AF; font-style: italic; padding: 4px;");
                objLayout->addWidget(noDataLabel);
            } else {
                table->setFixedHeight(
                    table->horizontalHeader()->height() + (table->rowHeight(0) * row) + 2);
                objLayout->addWidget(table);
            }
        } else {
            QLabel *noMembers = new QLabel(QString::fromUtf8("  (无成员)"));
            noMembers->setStyleSheet("color: #9CA3AF; font-style: italic; padding: 4px;");
            objLayout->addWidget(noMembers);
        }

        // 虚表信息
        if (!obj.vtable.empty()) {
            QLabel *vtableLabel = new QLabel(QString::fromUtf8("📋 VTable:"));
            vtableLabel->setStyleSheet("color: #7C3AED; font-weight: bold; font-size: 12px; padding: 2px 8px;");
            objLayout->addWidget(vtableLabel);
            for (const auto& vt : obj.vtable) {
                QLabel *entry = new QLabel("  " + QString::fromStdString(vt.first) +
                    " → " + QString::fromStdString(vt.second));
                entry->setStyleSheet("color: #6D28D9; font-size: 11px; padding: 1px 16px;");
                objLayout->addWidget(entry);
            }
        }

        mainHeapLayout->addWidget(objBox);
    }
}

void MainWindow::renderClassViews(const std::vector<ClassView>& classViews)
{
    clearLayout(mainClassLayout);

    if (classViews.empty()) {
        QLabel *emptyLabel = new QLabel(QString::fromUtf8("  (暂无类定义)"));
        emptyLabel->setStyleSheet("color: #9CA3AF; font-style: italic; padding: 4px;");
        mainClassLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto& cv : classViews) {
        // 每个类一个 GroupBox，绿色风格
        QString title = QString::fromStdString(cv.classname);
        if (!cv.baseClass.empty()) {
            title += QString::fromUtf8(" 继承自 ") + QString::fromStdString(cv.baseClass);
        }
        QGroupBox *classBox = new QGroupBox(title);
        classBox->setStyleSheet(R"(
            QGroupBox {
                background-color: #F0FDF4;
                border: 1px solid #86EFAC;
                border-radius: 8px;
                margin-top: 24px;
                padding-top: 10px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 15px;
                top: 5px;
                color: #166534;
                font-weight: bold;
                font-size: 13px;
                background-color: transparent;
            }
        )");

        QVBoxLayout *classLayout = new QVBoxLayout(classBox);

        bool hasContent = false; // 追踪是否有任何内容被添加

        // 继承深度
        if (cv.inheritance_depth > 0) {
            QLabel *depthLabel = new QLabel(
                QString::fromUtf8("继承深度: %1").arg(cv.inheritance_depth));
            depthLabel->setStyleSheet("color: #15803D; font-size: 12px; padding: 2px 8px;");
            classLayout->addWidget(depthLabel);
            hasContent = true;
        }

        // 派生类
        if (!cv.derived_classes.empty()) {
            QString derivedStr = QString::fromUtf8("派生类: ");
            for (size_t i = 0; i < cv.derived_classes.size(); ++i) {
                if (i > 0) derivedStr += ", ";
                derivedStr += QString::fromStdString(cv.derived_classes[i]);
            }
            QLabel *derivedLabel = new QLabel(derivedStr);
            derivedLabel->setStyleSheet("color: #15803D; font-size: 12px; padding: 2px 8px;");
            classLayout->addWidget(derivedLabel);
            hasContent = true;
        }

        // 数据成员
        bool hasDataMembers = false;
        for (const auto& m : cv.members) {
            if (!m.isMethod) { hasDataMembers = true; break; }
        }

        if (hasDataMembers) {
            QLabel *memSectionLabel = new QLabel(QString::fromUtf8("📦 数据成员:"));
            memSectionLabel->setStyleSheet("color: #166534; font-weight: bold; font-size: 12px; padding: 2px 8px;");
            classLayout->addWidget(memSectionLabel);

            QTableWidget *memTable = new QTableWidget();
            memTable->setColumnCount(3);
            memTable->setHorizontalHeaderLabels(
                QStringList() << QString::fromUtf8("名称") << QString::fromUtf8("类型") << QString::fromUtf8("值"));

            memTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            memTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            memTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
            memTable->verticalHeader()->setVisible(false);
            memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
            memTable->setSelectionMode(QAbstractItemView::NoSelection);

            int row = 0;
            for (const auto& m : cv.members) {
                if (m.isMethod) continue;
                memTable->setRowCount(row + 1);
                memTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(m.name)));
                memTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(m.type)));
                memTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(m.value)));
                ++row;
            }
            if (row > 0) {
                memTable->setFixedHeight(
                    memTable->horizontalHeader()->height() + (memTable->rowHeight(0) * row) + 2);
                classLayout->addWidget(memTable);
                hasContent = true;
            } else {
                delete memTable;
            }
        }

        // 方法
        bool hasMethods = false;
        for (const auto& m : cv.methods) {
            if (m.isMethod) { hasMethods = true; break; }
        }

        if (hasMethods) {
            QLabel *methSectionLabel = new QLabel(QString::fromUtf8("⚙️ 方法:"));
            methSectionLabel->setStyleSheet("color: #166534; font-weight: bold; font-size: 12px; padding: 2px 8px;");
            classLayout->addWidget(methSectionLabel);
            hasContent = true;

            for (const auto& m : cv.methods) {
                if (!m.isMethod) continue;
                QString methodSig = "  " + QString::fromStdString(m.name) +
                    "(" + QString::fromStdString(m.type) + ")";
                if (!m.value.empty()) {
                    methodSig += " → " + QString::fromStdString(m.value);
                }
                QLabel *methodLabel = new QLabel(methodSig);
                methodLabel->setStyleSheet("color: #15803D; font-size: 12px; padding: 1px 16px;");
                classLayout->addWidget(methodLabel);
            }
        }

        // 虚表
        if (!cv.vtable.empty()) {
            QLabel *vtableLabel = new QLabel(QString::fromUtf8("📋 虚函数表VTable:"));
            vtableLabel->setStyleSheet("color: #7C3AED; font-weight: bold; font-size: 12px; padding: 2px 8px;");
            classLayout->addWidget(vtableLabel);
            hasContent = true;
            for (const auto& vt : cv.vtable) {
                QLabel *entry = new QLabel("  " + QString::fromStdString(vt.first) +
                    " → " + QString::fromStdString(vt.second));
                entry->setStyleSheet("color: #6D28D9; font-size: 11px; padding: 1px 16px;");
                classLayout->addWidget(entry);
            }
        }

        // 只有当有内容时才添加 classBox，避免空白区域
        if (hasContent) {
            mainClassLayout->addWidget(classBox);
        } else {
            delete classBox; // 无内容时删除，避免内存泄漏
        }
    }
}

void MainWindow::on_actionFontSize_triggered()
{
    if (!m_codeEditor) return;

    bool ok = false;
    QFont currentFont = m_codeEditor->font();
    int currentSize = currentFont.pointSize();

    // 弹出输入对话框让用户选择字体大小
    int newSize = QInputDialog::getInt(
        this,
        QString::fromUtf8("设置字体大小"),
        QString::fromUtf8("请输入字体大小 (pt):"),
        currentSize,
        6,   // 最小值
        48,  // 最大值
        1,   // 步长
        &ok
    );

    if (ok && newSize != currentSize) {
        currentFont.setPointSize(newSize);
        m_codeEditor->setFont(currentFont);
        // 刷新行号区域宽度
        m_codeEditor->updateLineNumberAreaWidth(0);
    }
}

void MainWindow::on_actionFontColor_triggered()
{
    if (!m_codeEditor) return;

    // 获取当前字体颜色
    QColor currentColor = m_codeEditor->palette().color(QPalette::Text);
    if (!currentColor.isValid()) {
        currentColor = QColor("#D4D4D4"); // 默认VS Code风格浅灰白色
    }

    // 弹出颜色选择对话框
    QColor newColor = QColorDialog::getColor(
        currentColor,
        this,
        QString::fromUtf8("设置字体颜色")
    );

    if (newColor.isValid()) {
        // 通过 mergeCurrentCharFormat 设置文字颜色
        // 先更新文档的默认格式，再应用到已有内容
        QTextCharFormat fmt;
        fmt.setForeground(newColor);
        m_codeEditor->mergeCurrentCharFormat(fmt);
        // 同时更新默认格式，使新输入的文字也使用新颜色
        QTextCharFormat defaultFmt = m_codeEditor->currentCharFormat();
        defaultFmt.setForeground(newColor);
        m_codeEditor->setCurrentCharFormat(defaultFmt);
    }
}
void MainWindow::on_actionUserGuide_triggered()
{
    QDialog *guideDialog = new QDialog(this);
    guideDialog->setWindowTitle(QString::fromUtf8("用户必读"));
    guideDialog->setMinimumSize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(guideDialog);

    QTextBrowser *contentBrowser = new QTextBrowser(guideDialog);
    contentBrowser->setOpenExternalLinks(true);

    QString guideContent = QString::fromUtf8(
        "<h2>欢迎使用 码上搞定pro</h2>"
        "<hr/>"
        "<p>1.本产品由码上搞定队开发</p>"
        "<p>2.本产品由程序可视化界面和错题本构成。程序可视化界面支持变量，栈帧，静态类的可视化，可以分析每一步的状态，红色箭头代表当前的正在执行的语句，绿色箭头代表接下来要执行的语句。既可以用按键控制，也可以直接拖动进度条。当你要重新输入新的程序，直接点击开始状态就会更新。但是对于main函数和类的对象暂时不能兼容（还在持续优化中哦~）</p>"
        "<p>3.错题本支持自定义错题类型和知识库查询，可通过右键新建,重命名和删除</p>"
        "<p>4.快捷键：ctrl+首字母。如ctrl+p:previous,ctrl+f:first,ctrl+n:next,ctrl+l:last,ctrl+k:knowledge打开知识库</p>"
        "<p>最后送上github链接，快快点亮小星星吧！"
        "<a href='https://github.com/rainzyx1114/ProgrammingDesign-FinalWork'>码上搞定</a>"
        "</p>"
    );

    contentBrowser->setHtml(guideContent);
    layout->addWidget(contentBrowser);

    QPushButton *closeBtn = new QPushButton(QString::fromUtf8("关闭"), guideDialog);
    connect(closeBtn, &QPushButton::clicked, guideDialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    guideDialog->setAttribute(Qt::WA_DeleteOnClose);
    guideDialog->exec();
}

void MainWindow::on_actionKnowledgeBook_triggered()
{
    KnowledgeBookWidget *dialog = new KnowledgeBookWidget(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}