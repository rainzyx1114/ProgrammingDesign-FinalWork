#include "knowledgebookwidget.h"
#include "./ui_knowledgebookwidget.h"

#include <QStandardItemModel>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDate>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QColor>
#include <QCloseEvent>
#include <QMap>
#include <functional>
#include <algorithm>
#include <QRegularExpression>
#include <QDebug>
#include <QSet>
#include "markdownparser.h"
#include <QFileDialog>
#include <QTextCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPixmap>

KnowledgeBookWidget::KnowledgeBookWidget(QWidget *parent)
     : QDialog(parent)
     , ui(new Ui::KnowledgeBookWidget)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    ui->tabWidget->setCurrentIndex(0);

    // ========== 知识库初始化 ==========

    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);

    ui->textEdit->setReadOnly(true);
    ui->editButton->setVisible(false);
    ui->saveButton->setVisible(false);
    // 删除 newButton、deleteButton 及其 connect、visible 设置

    currentEditIndex = QPersistentModelIndex();
    currentErrorEditIndex = QPersistentModelIndex();

    // 创建模型
    kbModel = new QStandardItemModel(this);
    kbModel->setHorizontalHeaderLabels(QStringList() << "知识库目录");

    QDir().mkpath("./KnowledgeBase");
    QDir().mkpath("./ErrorNotebook");

    // 递归扫描
    scanFolder("./KnowledgeBase", kbModel->invisibleRootItem());

    // 设置视图
    ui->treeView->setModel(kbModel);
    ui->treeView->expandAll();
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->viewport()->installEventFilter(this);

    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        onTreeItemClicked(current);
    });
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &KnowledgeBookWidget::onTreeContextMenu);
    connect(ui->editButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onEditClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onSaveClicked);

    // ========== 错题本初始化 ==========

    ui->errorSplitter->setStretchFactor(0, 1);
    ui->errorSplitter->setStretchFactor(1, 4);

    ui->errorTextEdit->setReadOnly(true);
    ui->errorEditButton->setVisible(false);
    ui->errorSaveButton->setVisible(false);

    errorModel = new QStandardItemModel(this);
    errorModel->setHorizontalHeaderLabels(QStringList() << "错题本目录");

    scanFolder("./ErrorNotebook", errorModel->invisibleRootItem());

    ui->errorTreeView->setModel(errorModel);
    ui->errorTreeView->expandAll();
    ui->errorTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->errorTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->errorTreeView->viewport()->installEventFilter(this);

    ui->errorMetaGroupBox->setCheckable(true);
    ui->errorMetaGroupBox->setChecked(false);  // 默认折叠

    // 连接 toggled 信号，点击标题时折叠/展开内容
    connect(ui->errorMetaGroupBox, &QGroupBox::toggled, this, [this](bool checked) {
        // 遍历 GroupBox 里的子控件，控制显隐
        ui->errorTypeLabel->setVisible(checked);
        ui->errorKnowledgeLabel->setVisible(checked);
        ui->scanButton->setVisible(checked && !ui->errorTextEdit->isReadOnly());
        ui->errorTimeLabel->setVisible(checked);
    });

    // 初始状态：折叠，隐藏内容
    ui->errorTypeLabel->setVisible(false);
    ui->errorKnowledgeLabel->setVisible(false);
    ui->errorKnowledgeLabel->setWordWrap(true);
    ui->scanButton->setVisible(false);
    ui->scanButton->setMaximumWidth(95);
    ui->errorTimeLabel->setVisible(false);
    ui->errorMetaGroupBox->setVisible(false);

    connect(ui->errorTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        onErrorTreeItemClicked(current);
    });
    connect(ui->errorTreeView, &QTreeView::customContextMenuRequested, this, &KnowledgeBookWidget::onErrorTreeContextMenu);
    connect(ui->errorEditButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onErrorEditClicked);
    connect(ui->errorSaveButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onErrorSaveClicked);
    // 搜索按钮
    connect(ui->searchButton, &QPushButton::clicked, this, [this]() { onSearchButtonClicked(true); });
    connect(ui->errorSearchButton, &QPushButton::clicked, this, [this]() { onSearchButtonClicked(false); });

    // 收集所有知识点名称和路径（用于搜索）
    collectAllItems("./KnowledgeBase", allKnowledgeItems);
    collectAllItems("./ErrorNotebook", allErrorItems);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (isSwitchingTab) return;
        isSwitchingTab = true;

        if (index != 0 && !ui->textEdit->isReadOnly()) {
            ui->tabWidget->setCurrentIndex(0);
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "提示", "知识库内容尚未保存，是否保存？",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes) {
                onSaveClicked();
                isSwitchingTab = false;
                ui->tabWidget->setCurrentIndex(1);
                return;
            } else if (reply == QMessageBox::Cancel) {
                isSwitchingTab = false;
                return;
            }
            ui->textEdit->setReadOnly(true);
            ui->textEdit->clearFocus();
            ui->editButton->setEnabled(true);
            isSwitchingTab = false;
            ui->tabWidget->setCurrentIndex(1);
            return;
        }

        if (index != 1 && !ui->errorTextEdit->isReadOnly()) {
            ui->tabWidget->setCurrentIndex(1);
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "提示", "错题本内容尚未保存，是否保存？",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes) {
                onErrorSaveClicked();
                isSwitchingTab = false;
                ui->tabWidget->setCurrentIndex(0);
                return;
            } else if (reply == QMessageBox::Cancel) {
                isSwitchingTab = false;
                return;
            }
            ui->errorTextEdit->setReadOnly(true);
            ui->errorTextEdit->clearFocus();
            ui->errorEditButton->setEnabled(true);
            isSwitchingTab = false;
            ui->tabWidget->setCurrentIndex(1);
            return;
        }

        if (ui->tabWidget->currentIndex() == 1) {
            QModelIndex idx = ui->errorTreeView->currentIndex();
            if (idx.isValid()) {
                onErrorTreeItemClicked(idx);
            }
        }

        isSwitchingTab = false;
    });

    connect(ui->errorStatsButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onStatsButtonClicked);

    ui->scanButton->setVisible(false);
    connect(ui->scanButton, &QPushButton::clicked, this, &KnowledgeBookWidget::onScanButtonClicked);

    ui->insertImageButton->setVisible(false);
    ui->cleanButton->setVisible(false);
    ui->errorInsertImageButton->setVisible(false);
    ui->errorCleanButton->setVisible(false);
    connect(ui->insertImageButton, &QPushButton::clicked, this, [this]() { onInsertImageClicked(true); });
    connect(ui->cleanButton, &QPushButton::clicked, this, [this]() { onCleanImagesClicked(true); });
    connect(ui->errorInsertImageButton, &QPushButton::clicked, this, [this]() { onInsertImageClicked(false); });
    connect(ui->errorCleanButton, &QPushButton::clicked, this, [this]() { onCleanImagesClicked(false); });
}

KnowledgeBookWidget::~KnowledgeBookWidget()
{
    delete ui;
}

void KnowledgeBookWidget::closeEvent(QCloseEvent *event)
{
    static bool closing = false;
    if (closing) {
        event->accept();
        return;
    }

    bool needDelay = false;

    if (!ui->textEdit->isReadOnly()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "提示", "知识库内容尚未保存，是否保存？",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) {
            onSaveClicked();
            needDelay = true;
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    if (!ui->errorTextEdit->isReadOnly()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "提示", "错题本内容尚未保存，是否保存？",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) {
            onErrorSaveClicked();
            needDelay = true;
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    if (needDelay) {
        closing = true;
        event->ignore();
        QTimer::singleShot(300, this, [this]() {
            this->close();
        });
    } else {
        event->accept();
    }
}

bool KnowledgeBookWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (searchPopup && searchPopup->isVisible()) {
            QPoint pos = mouseEvent->globalPosition().toPoint();
            if (!searchPopup->geometry().contains(pos)) {
                hideSearchPopup();
                return false;
            }
        }
        if (statsPopup && statsPopup->isVisible()) {
            QMouseEvent *mouseEvent2 = static_cast<QMouseEvent*>(event);
            QPoint pos = mouseEvent2->globalPosition().toPoint();
            if (!statsPopup->geometry().contains(pos) &&
                !ui->errorStatsButton->geometry().contains(ui->errorStatsButton->parentWidget()->mapFromGlobal(pos))) {
                statsPopup->close();
                statsPopup->deleteLater();
                statsPopup = nullptr;
                statsResults = nullptr;
                statsVisible = false;
                ui->errorStatsButton->setText("📊 统计推荐");
                qApp->removeEventFilter(this);
            }
        }
        if (knowledgeSelectPopup && knowledgeSelectPopup->isVisible()) {
            QMouseEvent *mouseEvent3 = static_cast<QMouseEvent*>(event);
            QPoint pos = mouseEvent3->globalPosition().toPoint();
            if (!knowledgeSelectPopup->geometry().contains(pos) &&
                !ui->scanButton->geometry().contains(ui->scanButton->parentWidget()->mapFromGlobal(pos))) {
                knowledgeSelectPopup->close();
                knowledgeSelectPopup->deleteLater();
                knowledgeSelectPopup = nullptr;
                knowledgeSelectList = nullptr;
                knowledgeSelectVisible = false;
                ui->scanButton->setText("📋 浏览知识库");
                qApp->removeEventFilter(this);
            }
        }
    }

    if (event->type() == QEvent::ContextMenu) {
        // 拦截右键菜单事件，交给我们的槽函数处理
        QContextMenuEvent *ctxEvent = static_cast<QContextMenuEvent*>(event);
        if (obj == ui->treeView->viewport()) {
            onTreeContextMenu(ctxEvent->pos());
        } else if (obj == ui->errorTreeView->viewport()) {
            onErrorTreeContextMenu(ctxEvent->pos());
        }
        return true;  // 事件已处理，不再传递
    }
    return QWidget::eventFilter(obj, event);
}

// ==================== 递归扫描 ====================

void KnowledgeBookWidget::scanFolder(const QString &path, QStandardItem *parentItem)
{
    QDir dir(path);
    if (!dir.exists()) return;

    // 子文件夹
    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    foreach (const QString &subDir, subDirs) {
        QStandardItem *dirItem = new QStandardItem(subDir);
        parentItem->appendRow(dirItem);
        scanFolder(dir.filePath(subDir), dirItem);
    }

    // .md 文件
    QStringList mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files, QDir::Name);
    foreach (const QString &mdFile, mdFiles) {
        QString name = mdFile;
        name.chop(3);
        QStandardItem *fileItem = new QStandardItem(name);
        fileItem->setData(dir.filePath(mdFile), Qt::UserRole);
        // 衬底已取消
        parentItem->appendRow(fileItem);
    }
}

// ==================== 路径构建 ====================

QString KnowledgeBookWidget::buildFolderPath(QStandardItem *item, bool isKnowledgeBase) const
{
    QStringList parts;
    QStandardItem *current = item;
    QStandardItem *root = isKnowledgeBase ? kbModel->invisibleRootItem() : errorModel->invisibleRootItem();

    while (current && current != root) {
        parts.prepend(current->text());
        current = current->parent();
    }

    QString base = isKnowledgeBase ? "./KnowledgeBase" : "./ErrorNotebook";
    return base + "/" + parts.join("/");
}

// ==================== 知识库 点击 ====================

void KnowledgeBookWidget::onTreeItemClicked(const QModelIndex &index)
{
    if (!checkSaveBeforeAction(true)) return;

    ui->textEdit->setReadOnly(true);
    ui->textEdit->clearFocus();

    QStandardItem *item = kbModel->itemFromIndex(index);
    if (!item) {
        ui->textEdit->clear();
        ui->editButton->setVisible(false);
        ui->saveButton->setVisible(false);
        return;
    }

    QString filePath = item->data(Qt::UserRole).toString();

    if (!filePath.isEmpty()) {
        // 知识点
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            currentKbMarkdown = in.readAll();
            QString dir = QFileInfo(filePath).absolutePath();
            ui->textEdit->setHtml(MarkdownParser::toHtml(currentKbMarkdown, dir));
            file.close();
        }
        ui->editButton->setVisible(true);
        ui->saveButton->setVisible(true);
    } else {
        // 分类
        ui->textEdit->clear();
        ui->editButton->setVisible(false);
        ui->saveButton->setVisible(false);
    }
}

// ==================== 知识库 编辑/保存 ====================

void KnowledgeBookWidget::onEditClicked()
{
    currentEditIndex = ui->treeView->currentIndex();
    ui->textEdit->setReadOnly(false);
    ui->textEdit->setPlainText(currentKbMarkdown);
    ui->textEdit->setFocus();
    ui->editButton->setEnabled(false);
    ui->insertImageButton->setVisible(true);
    ui->cleanButton->setVisible(true);
}

void KnowledgeBookWidget::onSaveClicked()
{
    if (ui->textEdit->isReadOnly()) return;

    QStandardItem *item = kbModel->itemFromIndex(currentEditIndex);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->textEdit->toPlainText();
        file.close();
        QString dir = QFileInfo(filePath).absolutePath();
        currentKbMarkdown = ui->textEdit->toPlainText();
        ui->textEdit->setHtml(MarkdownParser::toHtml(currentKbMarkdown, dir));
        ui->textEdit->setReadOnly(true);
        ui->textEdit->clearFocus();
        ui->insertImageButton->setVisible(false);
        ui->cleanButton->setVisible(false);
        ui->statusLabel->setText("已保存");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        ui->editButton->setEnabled(true);
    } else {
        ui->statusLabel->setText("保存失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    }
}

// ==================== 知识库 右键菜单 ====================

void KnowledgeBookWidget::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->treeView->indexAt(pos);
    if (index.isValid()) {
        ui->treeView->setCurrentIndex(index);
    } else {
        ui->treeView->clearSelection();  // 空白处清除选中
        ui->treeView->setCurrentIndex(QModelIndex());  // 清空当前索引
    }

    QStandardItem *item = index.isValid() ? kbModel->itemFromIndex(index) : nullptr;
    bool isFile = item && !item->data(Qt::UserRole).toString().isEmpty();
    bool isFolder = item && item->data(Qt::UserRole).toString().isEmpty();

    QMenu menu(this);
    QAction *newCategory = nullptr;
    QAction *newItem = nullptr;
    QAction *rename = nullptr;
    QAction *moveTo = nullptr;
    QAction *del = nullptr;

    if (isFolder) {
        newCategory = menu.addAction("新建分类");
        newItem = menu.addAction("新建知识点");
        rename = menu.addAction("重命名");
        moveTo = menu.addAction("移动到...");
        del = menu.addAction("删除分类");
    } else if (isFile) {
        rename = menu.addAction("重命名");
        moveTo = menu.addAction("移动到...");
        del = menu.addAction("删除知识点");
    } else {
        // 空白处
        newCategory = menu.addAction("新建分类");
        newItem = menu.addAction("新建知识点");
    }

    QAction *chosen = menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == newCategory) {
        createCategory(true);
    } else if (newItem && chosen == newItem) {
        createKnowledgeItem(true);
    } else if (rename && chosen == rename) {
        renameItem(true);
    } else if (del && chosen == del) {
        deleteItem(true);
    } else if (moveTo && chosen == moveTo) {
        moveItem(true);
    }
}

// ==================== 错题本 点击 ====================

void KnowledgeBookWidget::onErrorTreeItemClicked(const QModelIndex &index)
{
    if (!checkSaveBeforeAction(false)) return;

    ui->errorTextEdit->setReadOnly(true);
    ui->errorTextEdit->clearFocus();

    QStandardItem *item = errorModel->itemFromIndex(index);
    if (!item) {
        ui->errorTextEdit->clear();
        ui->errorEditButton->setVisible(false);
        ui->errorSaveButton->setVisible(false);
        ui->errorMetaGroupBox->setVisible(false);
        return;
    }

    QString filePath = item->data(Qt::UserRole).toString();

    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString fullContent = file.readAll();
            file.close();

            // 解析 YAML front matter
            QString errorType = "/";
            QString knowledge = "/";
            QString time = "/";
            QString body = fullContent;

            if (fullContent.startsWith("---\n")) {
                int endIdx = fullContent.indexOf("\n---\n", 4);
                if (endIdx != -1) {
                    QString yamlBlock = fullContent.mid(4, endIdx - 4);
                    body = fullContent.mid(endIdx + 6);

                    QStringList lines = yamlBlock.split("\n");
                    for (const QString &line : std::as_const(lines)) {
                        if (line.startsWith("错误类型：")) {
                            errorType = line.mid(QString("错误类型：").length()).trimmed();
                        } else if (line.startsWith("关联知识点：")) {
                            knowledge = line.mid(QString("关联知识点：").length()).trimmed();
                        } else if (line.startsWith("创建时间：")) {
                            time = line.mid(QString("创建时间：").length()).trimmed();
                        }
                    }
                }
            }

            // 填充元数据控件
            ui->errorTypeLabel->setText("错误类型：" + errorType);
            // 处理关联知识点，分离正常和失效
            selectedKnowledges.clear();
            invalidKnowledges.clear();
            QStringList normalList;
            QStringList invalidList;
            if (!knowledge.isEmpty() && knowledge != "/") {
                QStringList items = knowledge.split(QRegularExpression("[,，]"));
                for (const QString &item : std::as_const(items)) {
                    QString trimmed = item.trimmed();
                    if (trimmed.isEmpty()) continue;
                    bool found = false;
                    for (const auto &kp : std::as_const(allKnowledgeItems)) {
                        if (kp.first == trimmed) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        selectedKnowledges.append(trimmed);
                        normalList.append(trimmed);
                    } else {
                        invalidKnowledges.insert(trimmed);
                        invalidList.append(trimmed);
                    }
                }
            }
            QString displayKnowledge;
            if (!normalList.isEmpty()) {
                displayKnowledge += normalList.join(", ");
            }
            if (!invalidList.isEmpty()) {
                if (!displayKnowledge.isEmpty()) displayKnowledge += "; ";
                displayKnowledge += "⚠已失效: " + invalidList.join(", ");
            }
            if (displayKnowledge.isEmpty()) displayKnowledge = "/";
            ui->errorKnowledgeLabel->setText("关联知识点：" + displayKnowledge);

            currentErrorFilePath = filePath;
            ui->errorTimeLabel->setText("创建时间：" + time);

            // 样式：QLabel 灰底
            // 衬底已取消

            currentErrorMarkdown = body;
            QString dir = QFileInfo(filePath).absolutePath();
            ui->errorTextEdit->setHtml(MarkdownParser::toHtml(body, dir));
        }
        ui->errorEditButton->setVisible(true);
        ui->errorSaveButton->setVisible(true);
        ui->errorMetaGroupBox->setVisible(true);
        ui->errorMetaGroupBox->setChecked(false);  // 默认折叠
    } else {
        ui->errorTextEdit->clear();
        ui->errorEditButton->setVisible(false);
        ui->errorSaveButton->setVisible(false);
        ui->errorMetaGroupBox->setVisible(false);
    }
}

// ==================== 错题本 编辑/保存 ====================

void KnowledgeBookWidget::onErrorEditClicked()
{
    currentErrorEditIndex = ui->errorTreeView->currentIndex();
    ui->errorTextEdit->setReadOnly(false);
    ui->errorTextEdit->setPlainText(currentErrorMarkdown);
    ui->errorTextEdit->setFocus();
    ui->errorEditButton->setEnabled(false);
    ui->errorInsertImageButton->setVisible(true);
    ui->errorCleanButton->setVisible(true);

    // 展开元数据面板
    ui->errorMetaGroupBox->setChecked(true);
    ui->errorTypeLabel->setVisible(true);
    ui->errorKnowledgeLabel->setVisible(true);
    ui->errorTimeLabel->setVisible(true);
    ui->scanButton->setVisible(true);
}

void KnowledgeBookWidget::onErrorSaveClicked()
{
    if (ui->errorTextEdit->isReadOnly()) return;

    QStandardItem *item = errorModel->itemFromIndex(currentErrorEditIndex);
    if (!item) return;

    QString filePath = item->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    // 从控件取值（去掉前缀）
    QString errorType = ui->errorTypeLabel->text();
    if (errorType.startsWith("错误类型：")) {
        errorType = errorType.mid(QString("错误类型：").length());
    }
    QStringList allK = selectedKnowledges + invalidKnowledges.values();
    QString knowledge = allK.join(", ");
    if (knowledge.isEmpty()) knowledge = "/";
    QString time = ui->errorTimeLabel->text();
    if (time.startsWith("创建时间：")) {
        time = time.mid(QString("创建时间：").length());
    }

    // 没有 YAML 则自动补全
    if (errorType.isEmpty() || errorType == "/") {
        QStandardItem *parent = item->parent();
        QStringList parts;
        while (parent && parent != errorModel->invisibleRootItem()) {
            parts.prepend(parent->text());
            parent = parent->parent();
        }
        errorType = parts.join(" / ");
        if (errorType.isEmpty()) errorType = "/";
    }
    if (time.isEmpty() || time == "/") {
        time = QDate::currentDate().toString("yyyy-MM-dd");
    }
    if (knowledge.isEmpty() || knowledge == "/") {
        knowledge = "/";
    }

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "---\n";
        out << "错误类型：" << errorType << "\n";
        out << "关联知识点：" << knowledge << "\n";
        out << "创建时间：" << time << "\n";
        out << "---\n\n";
        out << ui->errorTextEdit->toPlainText();
        file.close();
        ui->errorTypeLabel->setText("错误类型：" + errorType);
        ui->errorTimeLabel->setText("创建时间：" + time);
        QString dir = QFileInfo(filePath).absolutePath();
        currentErrorMarkdown = ui->errorTextEdit->toPlainText();
        ui->errorTextEdit->setHtml(MarkdownParser::toHtml(currentErrorMarkdown, dir));
        ui->errorTextEdit->setReadOnly(true);
        ui->errorTextEdit->clearFocus();
        ui->errorInsertImageButton->setVisible(false);
        ui->errorCleanButton->setVisible(false);
        ui->statusLabel->setText("已保存");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        ui->errorEditButton->setEnabled(true);
        ui->errorMetaGroupBox->setChecked(false);
        ui->scanButton->setVisible(false);
    } else {
        ui->statusLabel->setText("保存失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    }
}

// ==================== 错题本 右键菜单 ====================

void KnowledgeBookWidget::onErrorTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->errorTreeView->indexAt(pos);
    if (index.isValid()) {
        ui->errorTreeView->setCurrentIndex(index);
    } else {
        ui->errorTreeView->clearSelection();  // 空白处清除选中
        ui->errorTreeView->setCurrentIndex(QModelIndex());  // 清空当前索引
    }

    QStandardItem *item = index.isValid() ? errorModel->itemFromIndex(index) : nullptr;
    bool isFile = item && !item->data(Qt::UserRole).toString().isEmpty();
    bool isFolder = item && item->data(Qt::UserRole).toString().isEmpty();

    QMenu menu(this);
    QAction *newCategory = nullptr;
    QAction *newItem = nullptr;
    QAction *rename = nullptr;
    QAction * moveTo = nullptr;
    QAction *del = nullptr;

    if (isFolder) {
        newCategory = menu.addAction("新建分类");
        newItem = menu.addAction("新建错题");
        rename = menu.addAction("重命名");
        moveTo = menu.addAction("移动到...");
        del = menu.addAction("删除分类");
    } else if (isFile) {
        rename = menu.addAction("重命名");
        moveTo = menu.addAction("移动到...");
        del = menu.addAction("删除错题");
    } else {
        // 空白处
        newCategory = menu.addAction("新建分类");
        newItem = menu.addAction("新建错题");
    }

    QAction *chosen = menu.exec(ui->errorTreeView->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == newCategory) {
        createCategory(false);
    } else if (newItem && chosen == newItem) {
        createKnowledgeItem(false);
    } else if (rename && chosen == rename) {
        renameItem(false);
    } else if (del && chosen == del) {
        deleteItem(false);
    } else if (moveTo && chosen == moveTo) {
        moveItem(false);
    }
}

// ==================== 右键菜单操作实现 ====================

void KnowledgeBookWidget::sortItemChildren(QStandardItem *item)
{
    if (!item) return;
    item->sortChildren(0);
}

bool KnowledgeBookWidget::checkSaveBeforeAction(bool isKnowledgeBase)
{
    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;

    if (!edit->isReadOnly()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "提示",
            "当前内容尚未保存，是否保存后继续？",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            if (isKnowledgeBase) onSaveClicked();
            else onErrorSaveClicked();
            ui->statusLabel->setText("已保存");
            QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
            return true;
        } else if (reply == QMessageBox::Cancel) {
            return false;
        }
    }
    if (isKnowledgeBase) ui->editButton->setEnabled(true);
    else ui->errorEditButton->setEnabled(true);
    return true;
}

void KnowledgeBookWidget::createCategory(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;

    // 确定父节点和路径
    QModelIndex index = view->currentIndex();
    QStandardItem *parentItem = model->invisibleRootItem();
    QString parentPath = isKnowledgeBase ? "./KnowledgeBase" : "./ErrorNotebook";

    if (index.isValid()) {
        QStandardItem *item = model->itemFromIndex(index);
        if (item && item->data(Qt::UserRole).toString().isEmpty()) {
            // 选中的是分类，作为父节点
            parentItem = item;
            parentPath = buildFolderPath(item, isKnowledgeBase);
        }
    }
    // 空白处或选中的是知识点 → 使用根目录

    bool ok;
    QString name = QInputDialog::getText(this, "新建分类", "请输入分类名称：", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
        edit->setReadOnly(true);
        edit->clearFocus();
        return;
    }

    QDir parentDir(parentPath);
    if (!parentDir.mkdir(name)) {
        ui->statusLabel->setText("创建分类失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        return;
    }

    QStandardItem *newItem = new QStandardItem(name);
    parentItem->appendRow(newItem);
    sortItemChildren(parentItem);
    view->expand(parentItem->index());

    ui->statusLabel->setText("已创建分类：" + name);
    QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });

    refreshAllItems();
}

void KnowledgeBookWidget::createKnowledgeItem(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;
    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;

    QModelIndex index = view->currentIndex();

    QStandardItem *categoryItem = model->invisibleRootItem();
    QString folderPath = isKnowledgeBase ? "./KnowledgeBase" : "./ErrorNotebook";

    if (index.isValid()) {
        QStandardItem *item = model->itemFromIndex(index);
        if (item) {
            categoryItem = item->data(Qt::UserRole).toString().isEmpty() ? item : item->parent();
            if (categoryItem && categoryItem != model->invisibleRootItem()) {
                folderPath = buildFolderPath(categoryItem, isKnowledgeBase);
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this, "新建", "请输入名称：", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
        edit->setReadOnly(true);
        edit->clearFocus();
        return;
    }

    QString filePath = folderPath + "/" + name + ".md";
    QString typeName;
    QStandardItem *temp = categoryItem;
    QStringList parts;
    while (temp && temp != model->invisibleRootItem()) {
        parts.prepend(temp->text());
        temp = temp->parent();
    }
    typeName = parts.join(" / ");
    if (typeName.isEmpty()) typeName = "/";

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        if (!isKnowledgeBase) {
            out << "---\n";
            out << "错误类型: " << typeName << "\n";
            out << "关联知识点: \n";
            out << "创建时间: " << QDate::currentDate().toString("yyyy-MM-dd") << "\n";
            out << "---\n\n";
        }
        out << "# " << name << "\n\n";
        file.close();
    } else {
        ui->statusLabel->setText("创建失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        return;
    }

    QStandardItem *newItem = new QStandardItem(name);
    newItem->setData(filePath, Qt::UserRole);
    // 衬底已取消
    categoryItem->appendRow(newItem);
    sortItemChildren(categoryItem);

    view->expand(categoryItem->index());
    view->setCurrentIndex(newItem->index());

    if (isKnowledgeBase) {
        QFile newFile(filePath);
        if (newFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&newFile);
            currentKbMarkdown = in.readAll();
            newFile.close();
        }
        edit->setPlainText(currentKbMarkdown);
        edit->setReadOnly(false);
        edit->setFocus();
        currentEditIndex = view->currentIndex();
        ui->editButton->setEnabled(false);
        ui->editButton->setVisible(true);
        ui->saveButton->setVisible(true);
        ui->insertImageButton->setVisible(true);
        ui->cleanButton->setVisible(true);
    } else {
        // 错题本：解析 YAML，只显示正文
        QFile newFile(filePath);
        if (newFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString fullContent = newFile.readAll();
            newFile.close();

            QString body = fullContent;
            QString errorType = typeName;
            QString knowledge = "/";
            QString time = QDate::currentDate().toString("yyyy-MM-dd");

            if (fullContent.startsWith("---\n")) {
                int endIdx = fullContent.indexOf("\n---\n", 4);
                if (endIdx != -1) {
                    body = fullContent.mid(endIdx + 6);
                }
            }

            currentErrorMarkdown = body;
            ui->errorTextEdit->setPlainText(body);
            ui->errorTypeLabel->setText("错误类型：" + errorType);
            ui->errorKnowledgeLabel->setText("关联知识点：/");
            selectedKnowledges.clear();
            invalidKnowledges.clear();
            ui->errorTimeLabel->setText("创建时间：" + time);
        }

        // 元数据面板样式
        // 衬底已取消

        ui->errorTextEdit->setReadOnly(false);
        ui->errorTextEdit->setFocus();

        ui->errorMetaGroupBox->setVisible(true);
        ui->errorMetaGroupBox->setChecked(true);
        ui->errorTypeLabel->setVisible(true);
        ui->errorKnowledgeLabel->setVisible(true);
        ui->scanButton->setVisible(true);
        ui->errorTimeLabel->setVisible(true);

        currentErrorEditIndex = view->currentIndex();
        ui->errorEditButton->setEnabled(false);
        ui->errorEditButton->setVisible(true);
        ui->errorSaveButton->setVisible(true);
        ui->errorInsertImageButton->setVisible(true);
        ui->errorCleanButton->setVisible(true);
    }

    refreshAllItems();
}

void KnowledgeBookWidget::updatePathsRecursive(QStandardItem *item, const QString &oldBase, const QString &newBase, bool isKnowledgeBase)
{
    for (int i = 0; i < item->rowCount(); i++) {
        QStandardItem *child = item->child(i);
        QString childOldPath = child->data(Qt::UserRole).toString();

        if (!childOldPath.isEmpty()) {
            // 是知识点，更新路径
            QString childNewPath = childOldPath;
            childNewPath.replace(0, oldBase.length(), newBase);
            child->setData(childNewPath, Qt::UserRole);
        } else {
            // 是子分类，递归
            QString childOldFolderPath = buildFolderPath(child, isKnowledgeBase);
            QString childNewFolderPath = childOldFolderPath;
            childNewFolderPath.replace(0, oldBase.length(), newBase);
            updatePathsRecursive(child, childOldFolderPath, childNewFolderPath, isKnowledgeBase);
        }
    }
}

void KnowledgeBookWidget::renameItem(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;

    QModelIndex index = view->currentIndex();
    if (!index.isValid()) return;

    QStandardItem *item = model->itemFromIndex(index);
    if (!item) return;

    bool isFile = !item->data(Qt::UserRole).toString().isEmpty();
    QString oldName = item->text();
    QString oldPath = isFile ? item->data(Qt::UserRole).toString() : buildFolderPath(item, isKnowledgeBase);

    bool ok;
    QString newName = QInputDialog::getText(this, "重命名", "请输入新名称：", QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty() || newName == oldName) {
        QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
        edit->setReadOnly(true);
        edit->clearFocus();
        return;
    }

    QString parentPath = QFileInfo(oldPath).path();
    QString newPath = parentPath + "/" + newName + (isFile ? ".md" : "");

    if (QFile::rename(oldPath, newPath)) {
        item->setText(newName);
        sortItemChildren(item->parent() ? item->parent() : model->invisibleRootItem());
        if (isFile) {
            item->setData(newPath, Qt::UserRole);
        } else {
            updatePathsRecursive(item, oldPath, newPath, isKnowledgeBase);
        }
        // 如果是错题本的分类，递归更新其下所有错题的 YAML 错误类型
        if (!isKnowledgeBase && !isFile) {
            std::function<void(QStandardItem*)> updateYamlRecursive = [&](QStandardItem *node) {
                for (int i = 0; i < node->rowCount(); i++) {
                    QStandardItem *child = node->child(i);
                    QString childPath = child->data(Qt::UserRole).toString();
                    if (!childPath.isEmpty()) {
                        QFile f(childPath);
                        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            QString content = f.readAll();
                            f.close();
                            if (content.startsWith("---\n")) {
                                int endIdx = content.indexOf("\n---\n", 4);
                                if (endIdx != -1) {
                                    QStandardItem *temp = child->parent();
                                    QStringList parts;
                                    while (temp && temp != errorModel->invisibleRootItem()) {
                                        parts.prepend(temp->text());
                                        temp = temp->parent();
                                    }
                                    QString newErrorType = parts.join(" / ");
                                    if (newErrorType.isEmpty()) newErrorType = "/";

                                    QString yamlBlock = content.mid(4, endIdx - 4);
                                    QStringList lines = yamlBlock.split("\n");
                                    QStringList newLines;
                                    for (const QString &line : std::as_const(lines)) {
                                        if (line.startsWith("错误类型：")) {
                                            newLines.append("错误类型：" + newErrorType);
                                        } else {
                                            newLines.append(line);
                                        }
                                    }
                                    QString newYaml = newLines.join("\n");
                                    QString newContent = "---\n" + newYaml + "\n---\n\n" + content.mid(endIdx + 6);

                                    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                        QTextStream out(&f);
                                        out << newContent;
                                        f.close();
                                    }
                                }
                            }
                        }
                    } else {
                        updateYamlRecursive(child);
                    }
                }
            };
            updateYamlRecursive(item);
        }
        ui->statusLabel->setText("已重命名");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    } else {
        ui->statusLabel->setText("重命名失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    }

    refreshAllItems();
}

void KnowledgeBookWidget::deleteItem(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;
    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;

    QModelIndex index = view->currentIndex();
    if (!index.isValid()) return;

    QStandardItem *item = model->itemFromIndex(index);
    if (!item) return;

    bool isFile = !item->data(Qt::UserRole).toString().isEmpty();
    QString name = item->text();

    QString msg = isFile ? "确定要删除「" + name + "」吗？\n此操作不可恢复！" : "确定要删除分类「" + name + "」及其所有内容吗？\n此操作不可恢复！";

    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", msg, QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
        edit->setReadOnly(true);
        edit->clearFocus();
        return;
    }

    // 删除文件系统
    if (isFile) {
        QFile::remove(item->data(Qt::UserRole).toString());
    } else {
        QString folderPath = buildFolderPath(item, isKnowledgeBase);
        QDir(folderPath).removeRecursively();
    }

    // 从模型中移除
    QStandardItem *parent = item->parent();
    if (parent) {
        parent->removeRow(item->row());
        if (parent) sortItemChildren(parent);
    } else {
        model->removeRow(item->row());
    }

    edit->clear();
    edit->setReadOnly(true);

    if (isKnowledgeBase) {
        ui->editButton->setVisible(false);
        ui->saveButton->setVisible(false);
    } else {
        ui->errorEditButton->setVisible(false);
        ui->errorSaveButton->setVisible(false);
        ui->errorMetaGroupBox->setVisible(false);
    }

    refreshAllItems();
}

QStringList KnowledgeBookWidget::collectAllCategories(QStandardItemModel *model, QStandardItem *excludeItem) const
{
    QStringList result;
    result << "(根目录)";

    for (int i = 0; i < model->rowCount(); i++) {
        QStandardItem *item = model->item(i);
        if (item != excludeItem) {
            collectCategoriesRecursive(item, "", excludeItem, result);
        }
    }

    return result;
}

void KnowledgeBookWidget::collectCategoriesRecursive(QStandardItem *item, const QString &prefix,QStandardItem *excludeItem, QStringList &result) const
{
    if (item == excludeItem) return;
    if (!item->data(Qt::UserRole).toString().isEmpty()) return; // 跳过知识点

    QString displayName = prefix.isEmpty() ? item->text() : prefix + " / " + item->text();
    result << displayName;

    for (int i = 0; i < item->rowCount(); i++) {
        QStandardItem *child = item->child(i);
        collectCategoriesRecursive(child, displayName, excludeItem, result);
    }
}

void KnowledgeBookWidget::moveItem(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;
    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;

    QModelIndex index = view->currentIndex();
    if (!index.isValid()) return;

    QStandardItem *item = model->itemFromIndex(index);
    if (!item) return;

    QString itemName = item->text();
    bool isFile = !item->data(Qt::UserRole).toString().isEmpty();

    // 收集可选目标分类
    QStringList categories = collectAllCategories(model, isFile ? nullptr : item);

    bool ok;
    QString chosen = QInputDialog::getItem(this, "移动到", "将「" + itemName + "」移动到：", categories, 0, false, &ok);
    if (!ok) {
        // 取消
        edit->setReadOnly(true);
        edit->clearFocus();
        return;
    }

    // 解析目标
    QStandardItem *targetParent = model->invisibleRootItem();
    QString targetFolderPath = isKnowledgeBase ? "./KnowledgeBase" : "./ErrorNotebook";

    if (chosen != "(根目录)") {
        // 按路径查找目标分类节点
        QStringList pathParts = chosen.split(" / ");
        QStandardItem *current = nullptr;
        for (int i = 0; i < model->rowCount(); i++) {
            if (model->item(i)->text() == pathParts[0]) {
                current = model->item(i);
                break;
            }
        }
        for (int i = 1; i < pathParts.size() && current; i++) {
            QStandardItem *found = nullptr;
            for (int j = 0; j < current->rowCount(); j++) {
                if (current->child(j)->text() == pathParts[i]) {
                    found = current->child(j);
                    break;
                }
            }
            current = found;
        }
        if (current) {
            targetParent = current;
            targetFolderPath = buildFolderPath(current, isKnowledgeBase);
        }
    }

    // 检查是否移动到同一个父节点下
    QStandardItem *oldParent = item->parent() ? item->parent() : model->invisibleRootItem();
    if (oldParent == targetParent) {
        edit->setReadOnly(true);
        edit->clearFocus();
        ui->statusLabel->setText("已在目标位置");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        return;
    }

    // 检查目标是否已有同名节点
    QString targetName = item->text();
    for (int i = 0; i < targetParent->rowCount(); i++) {
        if (targetParent->child(i)->text() == targetName) {
            edit->setReadOnly(true);
            edit->clearFocus();
            ui->statusLabel->setText("目标位置已存在同名项！");
            QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
            return;
        }
    }

    // 旧路径
    QString oldPath;
    if (isFile) {
        oldPath = item->data(Qt::UserRole).toString();
    } else {
        oldPath = buildFolderPath(item, isKnowledgeBase);
    }

    // 从旧父节点移除
    int oldRow = item->row();
    if (oldParent == model->invisibleRootItem()) {
        model->takeRow(oldRow);
    } else {
        oldParent->takeRow(oldRow);
    }

    // 加到新父节点
    targetParent->appendRow(item);

    // 新路径
    QString newPath;
    if (isFile) {
        newPath = targetFolderPath + "/" + item->text() + ".md";
        item->setData(newPath, Qt::UserRole);
        QDir().mkpath(QFileInfo(newPath).path());
        QFile::rename(oldPath, newPath);
        // 移动该文件引用的图片
        QDir oldDir(QFileInfo(oldPath).absolutePath());
        QDir newDir(QFileInfo(newPath).absolutePath());
        QFile file(newPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            QRegularExpression imageRe("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)");
            QRegularExpressionMatchIterator it = imageRe.globalMatch(content);
            bool contentChanged = false;
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString origRef = match.captured(0);       // 完整 ![](...)
                QString alt = match.captured(1);
                QString imgName = match.captured(2);
                QString fileName = QFileInfo(imgName).fileName();
                QString oldImgPath = oldDir.filePath(fileName);
                if (!QFile::exists(oldImgPath)) continue;

                QString newFileName = fileName;
                QString newImgPath = newDir.filePath(newFileName);
                int counter = 1;
                QFileInfo fi(fileName);
                while (QFile::exists(newImgPath)) {
                    newFileName = fi.completeBaseName() + "_" + QString::number(counter) + "." + fi.suffix();
                    newImgPath = newDir.filePath(newFileName);
                    counter++;
                }

                QFile::rename(oldImgPath, newImgPath);

                if (newFileName != fileName) {
                    QString newRef = "![" + alt + "](" + newFileName + ")";
                    content.replace(origRef, newRef);
                    contentChanged = true;
                }
            }

            if (contentChanged) {
                // 回写更新后的引用
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << content;
                    file.close();
                }
                QTextStream out(&file);
                out << content;
                file.close();
            }
        }
    } else {
        newPath = targetFolderPath + "/" + item->text();
        QDir().mkpath(QFileInfo(newPath).path());
        QDir dir;
        dir.rename(oldPath, newPath);
        // 递归更新子节点路径
        updatePathsRecursive(item, oldPath, newPath, isKnowledgeBase);
    }

    // 如果是错题本的文件，更新 YAML 中的错误类型
    if (!isKnowledgeBase && isFile) {
        QFile errorFile(newPath);
        if (errorFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = errorFile.readAll();
            errorFile.close();

            if (content.startsWith("---\n")) {
                int endIdx = content.indexOf("\n---\n", 4);
                if (endIdx != -1) {
                    // 构建新错误类型
                    QStandardItem *temp = targetParent;
                    QStringList parts;
                    while (temp && temp != errorModel->invisibleRootItem()) {
                        parts.prepend(temp->text());
                        temp = temp->parent();
                    }
                    QString newErrorType = parts.join(" / ");
                    if (newErrorType.isEmpty()) newErrorType = "/";

                    // 更新错误类型字段
                    QString yamlBlock = content.mid(4, endIdx - 4);
                    QStringList lines = yamlBlock.split("\n");
                    QStringList newLines;
                    for (const QString &line : std::as_const(lines)) {
                        if (line.startsWith("错误类型：")) {
                            newLines.append("错误类型：" + newErrorType);
                        } else {
                            newLines.append(line);
                        }
                    }
                    QString newYaml = newLines.join("\n");
                    QString newContent = "---\n" + newYaml + "\n---\n\n" + content.mid(endIdx + 6);

                    if (errorFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&errorFile);
                        out << newContent;
                        errorFile.close();
                    }
                }
            }
        }
    }

    // 如果是错题本的分类，递归更新其下所有错题的 YAML 错误类型
    if (!isKnowledgeBase && !isFile) {
        std::function<void(QStandardItem*)> updateYamlRecursive = [&](QStandardItem *node) {
            for (int i = 0; i < node->rowCount(); i++) {
                QStandardItem *child = node->child(i);
                QString childPath = child->data(Qt::UserRole).toString();
                if (!childPath.isEmpty()) {
                    // 是错题文件
                    QFile f(childPath);
                    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString content = f.readAll();
                        f.close();
                        if (content.startsWith("---\n")) {
                            int endIdx = content.indexOf("\n---\n", 4);
                            if (endIdx != -1) {
                                QStandardItem *temp = child->parent();
                                QStringList parts;
                                while (temp && temp != errorModel->invisibleRootItem()) {
                                    parts.prepend(temp->text());
                                    temp = temp->parent();
                                }
                                QString newErrorType = parts.join(" / ");
                                if (newErrorType.isEmpty()) newErrorType = "/";

                                QString yamlBlock = content.mid(4, endIdx - 4);
                                QStringList lines = yamlBlock.split("\n");
                                QStringList newLines;
                                for (const QString &line : std::as_const(lines)) {
                                    if (line.startsWith("错误类型：")) {
                                        newLines.append("错误类型：" + newErrorType);
                                    } else {
                                        newLines.append(line);
                                    }
                                }
                                QString newYaml = newLines.join("\n");
                                QString newContent = "---\n" + newYaml + "\n---\n\n" + content.mid(endIdx + 6);

                                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                    QTextStream out(&f);
                                    out << newContent;
                                    f.close();
                                }
                            }
                        }
                    }
                } else {
                    updateYamlRecursive(child);
                }
            }
        };
        updateYamlRecursive(item);
    }

    // 排序 + 展开目标
    sortItemChildren(targetParent);
    view->expand(targetParent->index());

    // 清空编辑区 + 切回阅读模式
    edit->clear();
    edit->setReadOnly(true);
    edit->clearFocus();
    if (isKnowledgeBase) {
        ui->editButton->setVisible(false);
        ui->saveButton->setVisible(false);
    }
    if (!isKnowledgeBase) {
        ui->errorMetaGroupBox->setVisible(false);
        ui->errorEditButton->setVisible(false);
        ui->errorSaveButton->setVisible(false);
    }
    ui->statusLabel->setText("已移动「" + itemName + "」→ " + chosen);
    QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });

    refreshAllItems();
}

// ==================== 搜索功能实现 ====================

void KnowledgeBookWidget::collectAllItems(const QString &dirPath, QList<QPair<QString, QString>> &list)
{
    QDir dir(dirPath);
    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &subDir, subDirs) {
        collectAllItems(dir.filePath(subDir), list);
    }
    QStringList mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files);
    foreach (const QString &mdFile, mdFiles) {
        QString name = mdFile;
        name.chop(3);
        QString relativePath = dir.filePath(mdFile);
        relativePath.replace("\\", "/");
        list.append({name, relativePath});
    }
}

void KnowledgeBookWidget::onSearchButtonClicked(bool isKnowledgeBase)
{
    if (!checkSaveBeforeAction(isKnowledgeBase)) return;

    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
    edit->setReadOnly(true);
    edit->clearFocus();

    currentSearchIsKnowledgeBase = isKnowledgeBase;
    showSearchPopup(isKnowledgeBase);
}

void KnowledgeBookWidget::showSearchPopup(bool isKnowledgeBase)
{
    if (searchPopup) hideSearchPopup();

    QWidget *parent = isKnowledgeBase ? ui->tab : ui->tab_2;

    searchPopup = new QWidget(parent, Qt::Tool);
    searchPopup->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    searchPopup->setAttribute(Qt::WA_InputMethodEnabled, true);
    searchPopup->setAttribute(Qt::WA_ShowWithoutActivating, true);
    searchPopup->setFocusPolicy(Qt::StrongFocus);
    searchPopup->setFixedSize(350, 40);  // 初始只显示输入框高度
    searchPopup->setMinimumWidth(350);

    QVBoxLayout *layout = new QVBoxLayout(searchPopup);
    layout->setContentsMargins(5, 5, 5, 5);

    searchInput = new QLineEdit(searchPopup);
    if (isKnowledgeBase) {
        searchInput->setPlaceholderText("搜索知识点...");
    } else {
        searchInput->setPlaceholderText("搜索错题...");
    }
    searchInput->setClearButtonEnabled(true);
    searchInput->setFixedHeight(30);
    layout->addWidget(searchInput);
    connect(searchInput, &QLineEdit::textChanged, this, &KnowledgeBookWidget::onSearchTextChanged);

    searchResults = new QListWidget(searchPopup);
    searchResults->setVisible(false);
    layout->addWidget(searchResults);

    connect(searchResults, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString filePath = item->data(Qt::UserRole).toString();
        onSearchResultClicked(filePath);
    });

    // 定位在搜索按钮下方
    QPushButton *btn = isKnowledgeBase ? ui->searchButton : ui->errorSearchButton;
    QPoint pos = btn->mapToGlobal(QPoint(0, btn->height()));
    searchPopup->move(pos);
    searchPopup->show();
    QTimer::singleShot(0, searchInput, [this]() {
        searchInput->setFocus();
        searchInput->activateWindow();
    });

    qApp->installEventFilter(this);
}

void KnowledgeBookWidget::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        searchResults->setVisible(false);
        searchInput->setFixedHeight(30);  // 确保输入框高度不变
        searchPopup->setFixedSize(350, 40);
        return;
    }

    searchResults->clear();
    const QList<QPair<QString, QString>> &source = currentSearchIsKnowledgeBase ? allKnowledgeItems : allErrorItems;

    for (const auto &pair : source) {
        const QString &name = pair.first;
        const QString &filePath = pair.second;

        // 搜索文件名
        if (name.contains(text, Qt::CaseInsensitive)) {
            QListWidgetItem *item = new QListWidgetItem(name + " — " + buildDisplayPath(filePath));
            item->setData(Qt::UserRole, filePath);
            searchResults->addItem(item);
            continue;
        }

        // 搜索文件内容
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            if (content.contains(text, Qt::CaseInsensitive)) {
                QListWidgetItem *item = new QListWidgetItem(name + " — " + buildDisplayPath(filePath));
                item->setData(Qt::UserRole, filePath);
                searchResults->addItem(item);
            }
        }
    }

    if (searchResults->count() > 0) {
        searchResults->setVisible(true);
        int h = qMin(searchResults->count() * 30, 200);
        searchPopup->setFixedSize(350, 40 + h + 10);
    } else {
        searchResults->setVisible(false);
        searchPopup->setFixedSize(350, 40);
    }
}

QString KnowledgeBookWidget::buildDisplayPath(const QString &filePath)
{
    QString path = filePath;
    path.replace("\\", "/");
    path.replace("./KnowledgeBase/", "");
    path.replace("./ErrorNotebook/", "");
    // 去掉文件名
    int lastSlash = path.lastIndexOf("/");
    if (lastSlash > 0) path = path.left(lastSlash);
    else path = "(根目录)";
    return path;
}

void KnowledgeBookWidget::onSearchResultClicked(const QString &filePath)
{
    hideSearchPopup();

    // 判断是知识库还是错题本
    bool isKnowledgeBase = filePath.contains("KnowledgeBase");

    // 切换到对应标签页
    ui->tabWidget->setCurrentIndex(isKnowledgeBase ? 0 : 1);

    // 在目录树中定位并选中该文件
    QStandardItemModel *model = isKnowledgeBase ? kbModel : errorModel;
    QTreeView *view = isKnowledgeBase ? ui->treeView : ui->errorTreeView;

    // 递归查找
    QStandardItem *found = findItemByPath(model->invisibleRootItem(), filePath);
    if (found) {
        // 展开所有父节点
        QStandardItem *parent = found->parent();
        while (parent && parent != model->invisibleRootItem()) {
            view->expand(parent->index());
            parent = parent->parent();
        }
        view->setCurrentIndex(found->index());
        if (isKnowledgeBase) onTreeItemClicked(found->index());
        else onErrorTreeItemClicked(found->index());
    }
}

QStandardItem* KnowledgeBookWidget::findItemByPath(QStandardItem *parent, const QString &filePath)
{
    for (int i = 0; i < parent->rowCount(); i++) {
        QStandardItem *child = parent->child(i);
        QString childPath = child->data(Qt::UserRole).toString();
        if (!childPath.isEmpty() && childPath == filePath) {
            return child;
        }
        QStandardItem *found = findItemByPath(child, filePath);
        if (found) return found;
    }
    return nullptr;
}

void KnowledgeBookWidget::hideSearchPopup()
{
    qApp->removeEventFilter(this);
    if (searchPopup) {
        searchPopup->close();
        searchPopup->deleteLater();
        searchPopup = nullptr;
        searchInput = nullptr;
        searchResults = nullptr;
    }
}

void KnowledgeBookWidget::refreshAllItems()
{
    allKnowledgeItems.clear();
    allErrorItems.clear();
    collectAllItems("./KnowledgeBase", allKnowledgeItems);
    collectAllItems("./ErrorNotebook", allErrorItems);
}

void KnowledgeBookWidget::onStatsButtonClicked()
{
    if (!checkSaveBeforeAction(false)) return;

    if (statsVisible) {
        if (statsPopup) {
            statsPopup->close();
            statsPopup->deleteLater();
            statsPopup = nullptr;
            statsResults = nullptr;
        }
        statsVisible = false;
        ui->errorStatsButton->setText("📊 统计推荐");
        qApp->removeEventFilter(this);
        return;
    }

    // 统计
    QMap<QString, int> countMap;

    for (const auto &pair : std::as_const(allErrorItems)) {
        QFile file(pair.second);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            if (content.startsWith("---\n")) {
                int endIdx = content.indexOf("\n---\n", 4);
                if (endIdx != -1) {
                    QString yamlBlock = content.mid(4, endIdx - 4);
                    QStringList lines = yamlBlock.split("\n");
                    for (const QString &line : std::as_const(lines)) {
                        if (line.startsWith("关联知识点：")) {
                            QString knowledge = line.mid(QString("关联知识点：").length()).trimmed();
                            if (!knowledge.isEmpty() && knowledge != "/") {
                                QStringList items = knowledge.split(QRegularExpression("[,，]"));
                                for (QString item : std::as_const(items)) {
                                    item = item.trimmed();
                                    if (!item.isEmpty()) {
                                        bool exists = false;
                                        for (const auto &kp : std::as_const(allKnowledgeItems)) {
                                            if (kp.first == item) {
                                                exists = true;
                                                break;
                                            }
                                        }
                                        if (exists) {
                                            countMap[item]++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 排序取 Top 3
    QList<QPair<QString, int>> sorted;
    for (auto it = countMap.begin(); it != countMap.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        return a.second > b.second;
    });
    if (sorted.size() > 3) sorted = sorted.mid(0, 3);

    // 弹出结果面板
    statsPopup = new QWidget(ui->tab_2, Qt::Tool);
    statsPopup->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    statsPopup->setFixedSize(300, sorted.isEmpty() ? 60 : sorted.size() * 35 + 50);

    QVBoxLayout *layout = new QVBoxLayout(statsPopup);
    layout->setContentsMargins(5, 5, 5, 5);

    QLabel *title = new QLabel("📊 高频出错知识点", statsPopup);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    if (sorted.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无统计数据", statsPopup);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
    } else {
        statsResults = new QListWidget(statsPopup);
        for (const auto &pair : std::as_const(sorted)) {
            QString displayPath = "(根目录)";
            for (const auto &kp : std::as_const(allKnowledgeItems)) {
                if (kp.first == pair.first) {
                    displayPath = buildDisplayPath(kp.second);
                    break;
                }
            }
            QListWidgetItem *item = new QListWidgetItem(pair.first + "（" + QString::number(pair.second) + "次） — " + displayPath);
            item->setData(Qt::UserRole, pair.first);
            statsResults->addItem(item);
        }
        layout->addWidget(statsResults);

        connect(statsResults, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            QString knowledgeName = item->data(Qt::UserRole).toString();
            for (const auto &pair : std::as_const(allKnowledgeItems)) {
                if (pair.first == knowledgeName) {
                    statsPopup->close();
                    statsPopup->deleteLater();
                    statsPopup = nullptr;
                    statsResults = nullptr;
                    statsVisible = false;
                    ui->errorStatsButton->setText("📊 统计推荐");
                    qApp->removeEventFilter(this);
                    onSearchResultClicked(pair.second);
                    return;
                }
            }
            ui->statusLabel->setText("未找到知识点：" + knowledgeName);
            QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        });
    }

    // 定位在按钮上方
    QPoint btnPos = ui->errorStatsButton->mapToGlobal(QPoint(0, 0));
    statsPopup->move(btnPos.x(), btnPos.y() - statsPopup->height());
    statsPopup->show();

    statsVisible = true;
    ui->errorStatsButton->setText("📊 收起统计");

    qApp->installEventFilter(this);
}

void KnowledgeBookWidget::onScanButtonClicked()
{
    if (knowledgeSelectVisible) {
        // 收起
        if (knowledgeSelectPopup) {
            knowledgeSelectPopup->close();
            knowledgeSelectPopup->deleteLater();
            knowledgeSelectPopup = nullptr;
            knowledgeSelectList = nullptr;
        }
        knowledgeSelectVisible = false;
        ui->scanButton->setText("📋 浏览知识库");
        return;
    }

    // 展开
    knowledgeSelectPopup = new QWidget(ui->tab_2, Qt::Tool);
    knowledgeSelectPopup->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    knowledgeSelectPopup->setFixedSize(350, 300);

    QVBoxLayout *layout = new QVBoxLayout(knowledgeSelectPopup);
    layout->setContentsMargins(5, 5, 5, 5);

    QLabel *title = new QLabel("选择关联知识点", knowledgeSelectPopup);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    knowledgeSelectList = new QListWidget(knowledgeSelectPopup);
    layout->addWidget(knowledgeSelectList);

    // 填充正常知识点
    for (const auto &pair : std::as_const(allKnowledgeItems)) {
        QString displayPath = buildDisplayPath(pair.second);
        QListWidgetItem *item = new QListWidgetItem(pair.first + " — " + displayPath);
        item->setData(Qt::UserRole, pair.first);
        item->setData(Qt::UserRole + 1, "normal");
        if (selectedKnowledges.contains(pair.first)) {
            item->setText("✓ " + pair.first + " — " + displayPath);
        }
        knowledgeSelectList->addItem(item);
    }

    // 分隔
    if (!invalidKnowledges.isEmpty()) {
        QListWidgetItem *sep = new QListWidgetItem("———— 已失效 ————");
        sep->setFlags(Qt::NoItemFlags);
        sep->setForeground(QColor(128, 128, 128));
        knowledgeSelectList->addItem(sep);

        for (const QString &name : std::as_const(invalidKnowledges)) {
            QListWidgetItem *item = new QListWidgetItem("⚠ " + name + "（点击删除）");
            item->setData(Qt::UserRole, name);
            item->setData(Qt::UserRole + 1, "invalid");
            item->setForeground(QColor(128, 128, 128));
            knowledgeSelectList->addItem(item);
        }
    }

    connect(knowledgeSelectList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (item->text() == "已删除") return;

        QString type = item->data(Qt::UserRole + 1).toString();
        QString name = item->data(Qt::UserRole).toString();

        if (type == "normal") {
            QString displayPath = "(根目录)";
            for (const auto &kp : std::as_const(allKnowledgeItems)) {
                if (kp.first == name) {
                    displayPath = buildDisplayPath(kp.second);
                    break;
                }
            }
            if (selectedKnowledges.contains(name)) {
                selectedKnowledges.removeAll(name);
                item->setText(name + " — " + displayPath);
            } else {
                if (!selectedKnowledges.contains(name)) {
                    selectedKnowledges.append(name);
                }
                item->setText("✓ " + name + " — " + displayPath);
            }
            // 更新 label
            QStringList normalList = selectedKnowledges;
            QStringList invalidList = invalidKnowledges.values();
            QString displayKnowledge;
            if (!normalList.isEmpty()) displayKnowledge += normalList.join(", ");
            if (!invalidList.isEmpty()) {
                if (!displayKnowledge.isEmpty()) displayKnowledge += "; ";
                displayKnowledge += "⚠已失效: " + invalidList.join(", ");
            }
            if (displayKnowledge.isEmpty()) displayKnowledge = "/";
            ui->errorKnowledgeLabel->setText("关联知识点：" + displayKnowledge);
        } else if (type == "invalid") {
            QString nameCopy = name;
            QTimer::singleShot(0, this, [this, nameCopy]() {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this, "删除", "确定要删除失效关联「" + nameCopy + "」吗？",
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    invalidKnowledges.remove(nameCopy);
                    QStringList normalList = selectedKnowledges;
                    QStringList invalidList = invalidKnowledges.values();
                    QString displayKnowledge;
                    if (!normalList.isEmpty()) displayKnowledge += normalList.join(", ");
                    if (!invalidList.isEmpty()) {
                        if (!displayKnowledge.isEmpty()) displayKnowledge += "; ";
                        displayKnowledge += "⚠已失效: " + invalidList.join(", ");
                    }
                    if (displayKnowledge.isEmpty()) displayKnowledge = "/";
                    ui->errorKnowledgeLabel->setText("关联知识点：" + displayKnowledge);
                    ui->statusLabel->setText("已删除失效关联：" + nameCopy);
                    QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
                }
            });
        }
    });

    // 定位在按钮旁边
    QPoint btnPos = ui->scanButton->mapToGlobal(QPoint(0, 0));
    knowledgeSelectPopup->move(btnPos.x() + ui->scanButton->width() - knowledgeSelectPopup->width(), btnPos.y() + ui->scanButton->height());
    knowledgeSelectPopup->show();

    knowledgeSelectVisible = true;
    ui->scanButton->setText("📋 收起列表");

    qApp->installEventFilter(this);
}

void KnowledgeBookWidget::onInsertImageClicked(bool isKnowledgeBase)
{
    QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
    QStandardItem *currentItem = isKnowledgeBase
                                     ? kbModel->itemFromIndex(ui->treeView->currentIndex())
                                     : errorModel->itemFromIndex(ui->errorTreeView->currentIndex());
    if (!currentItem) return;
    QString filePath = currentItem->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    QString dir = QFileInfo(filePath).absolutePath();

    QString imagePath = QFileDialog::getOpenFileName(this, "选择图片", "",
                                                     "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (imagePath.isEmpty()) return;

    QFileInfo fi(imagePath);
    QString fileName = fi.fileName();

    // 让用户确认文件名
    bool ok;
    QString newName = QInputDialog::getText(this, "图片名称", "请输入图片文件名：", QLineEdit::Normal,
                                            fi.completeBaseName(), &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        ui->statusLabel->setText("已取消插入图片");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
        return;
    }
    fileName = newName.trimmed() + "." + fi.suffix();

    QString destPath = dir + "/" + fileName;

    // 如果同名文件已存在，加后缀
    int counter = 1;
    QFileInfo newFi(fileName);
    while (QFile::exists(destPath)) {
        fileName = newFi.completeBaseName() + "_" + QString::number(counter) + "." + newFi.suffix();
        destPath = dir + "/" + fileName;
        counter++;
    }

    if (QFile::copy(imagePath, destPath)) {
        QTextCursor cursor = edit->textCursor();
        cursor.insertText("![](" + fileName + ")");
        ui->statusLabel->setText("图片已插入: " + fileName);
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    } else {
        ui->statusLabel->setText("图片插入失败！");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    }
}

void KnowledgeBookWidget::onCleanImagesClicked(bool isKnowledgeBase)
{
    QStandardItem *currentItem = isKnowledgeBase
                                     ? kbModel->itemFromIndex(ui->treeView->currentIndex())
                                     : errorModel->itemFromIndex(ui->errorTreeView->currentIndex());
    if (!currentItem) return;
    QString filePath = currentItem->data(Qt::UserRole).toString();
    if (filePath.isEmpty()) return;

    QString dir = QFileInfo(filePath).absolutePath();

    // 收集目录中所有 .md 文件的图片引用（磁盘 + 编辑区并集）
    QSet<QString> referencedImages;
    QRegularExpression imageRe("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)");
    QDir mdDir(dir);
    QStringList mdFiles = mdDir.entryList(QStringList() << "*.md", QDir::Files);
    QString currentMdFileName = QFileInfo(filePath).fileName();
    for (const QString &mdFile : std::as_const(mdFiles)) {
        QString content;
        if (mdFile == currentMdFileName) {
            // 当前文件：读取磁盘内容
            QFile f(dir + "/" + mdFile);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                content = f.readAll();
                f.close();
            }
            // 并上编辑区内容
            QTextEdit *edit = isKnowledgeBase ? ui->textEdit : ui->errorTextEdit;
            content += "\n" + edit->toPlainText();
        } else {
            QFile f(dir + "/" + mdFile);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                content = f.readAll();
                f.close();
            }
        }
        QRegularExpressionMatchIterator it = imageRe.globalMatch(content);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString imgPath = match.captured(2);
            QString fileName = QFileInfo(imgPath).fileName();
            referencedImages.insert(fileName);
        }
    }

    // 扫描目录中的图片文件
    QDir imgDir(dir);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.gif";
    QStringList imageFiles = imgDir.entryList(filters, QDir::Files);

    QStringList unreferenced;
    for (const QString &file : std::as_const(imageFiles)) {
        if (!referencedImages.contains(file)) {
            unreferenced.append(file);
        }
    }

    if (unreferenced.isEmpty()) {
        QMessageBox::information(this, "清理图片", "没有多余的图片文件。");
        return;
    }

    // 弹出对话框，显示未引用图片列表和预览
    QDialog dialog(this);
    dialog.setWindowTitle("清理多余图片");
    dialog.setFixedSize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *infoLabel = new QLabel("以下图片未被引用，勾选要删除的：");
    layout->addWidget(infoLabel);

    QListWidget *listWidget = new QListWidget(&dialog);
    for (const QString &file : unreferenced) {
        QListWidgetItem *item = new QListWidgetItem(file);
        item->setCheckState(Qt::Checked);
        listWidget->addItem(item);
    }
    layout->addWidget(listWidget);

    QLabel *previewLabel = new QLabel(&dialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(100);
    layout->addWidget(previewLabel);

    connect(listWidget, &QListWidget::currentItemChanged, this, [&](QListWidgetItem *current, QListWidgetItem *) {
        if (current) {
            QString imgPath = dir + "/" + current->text();
            QPixmap pix(imgPath);
            if (!pix.isNull()) {
                previewLabel->setPixmap(pix.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                previewLabel->setText("无法预览");
            }
        }
    });

    // 触发第一个预览
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int deletedCount = 0;
        for (int i = 0; i < listWidget->count(); i++) {
            QListWidgetItem *item = listWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                QString fileToDelete = dir + "/" + item->text();
                if (QFile::remove(fileToDelete)) {
                    deletedCount++;
                }
            }
        }
        ui->statusLabel->setText("已删除 " + QString::number(deletedCount) + " 张图片");
        QTimer::singleShot(2000, this, [this]() { ui->statusLabel->clear(); });
    }
}