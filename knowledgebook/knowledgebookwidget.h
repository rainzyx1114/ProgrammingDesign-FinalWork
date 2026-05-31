#ifndef KNOWLEDGEBOOKWIDGET_H
#define KNOWLEDGEBOOKWIDGET_H

#include <QDialog>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QListWidget>
#include <QPair>

namespace Ui { class KnowledgeBookWidget; }

class KnowledgeBookWidget : public QDialog
{
    Q_OBJECT

public:
    explicit KnowledgeBookWidget(QWidget *parent = nullptr);
    ~KnowledgeBookWidget() override;

private slots:
    // 知识库
    void onTreeItemClicked(const QModelIndex &index);
    void onEditClicked();
    void onSaveClicked();
    void onTreeContextMenu(const QPoint &pos);

    // 错题本
    void onErrorTreeItemClicked(const QModelIndex &index);
    void onErrorEditClicked();
    void onErrorSaveClicked();
    void onErrorTreeContextMenu(const QPoint &pos);

    //搜索
    void onSearchButtonClicked(bool isKnowledgeBase);
    void onSearchTextChanged(const QString &text);
    void onSearchResultClicked(const QString &filePath);

    void onStatsButtonClicked();

    void onScanButtonClicked();

    void onInsertImageClicked(bool isKnowledgeBase);
    void onCleanImagesClicked(bool isKnowledgeBase);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::KnowledgeBookWidget *ui;

    bool isSwitchingTab = false;
    QPersistentModelIndex currentEditIndex;
    QPersistentModelIndex currentErrorEditIndex;

    // 模型指针（多级分类递归需要）
    QStandardItemModel *kbModel;
    QStandardItemModel *errorModel;

    // 递归扫描文件夹
    void scanFolder(const QString &path, QStandardItem *parentItem);

    // 根据节点构建完整文件夹路径
    QString buildFolderPath(QStandardItem *item, bool isKnowledgeBase) const;

    // 右键菜单触发的操作
    void createCategory(bool isKnowledgeBase);
    void createKnowledgeItem(bool isKnowledgeBase);
    void renameItem(bool isKnowledgeBase);
    void deleteItem(bool isKnowledgeBase);
    bool checkSaveBeforeAction(bool isKnowledgeBase);
    void updatePathsRecursive(QStandardItem *item, const QString &oldBase, const QString &newBase, bool isKnowledgeBase);
    void sortItemChildren(QStandardItem *item);
    QStringList collectAllCategories(QStandardItemModel *model, QStandardItem *excludeItem) const;
    void collectCategoriesRecursive(QStandardItem *item, const QString &prefix, QStandardItem *excludeItem, QStringList &result) const;
    void moveItem(bool isKnowledgeBase);

    //搜索
    void showSearchPopup(bool isKnowledgeBase);
    void hideSearchPopup();
    void performSearch(const QString &keyword);
    void collectAllItems(const QString &dirPath, QList<QPair<QString, QString>> &list);
    QString buildDisplayPath(const QString &filePath);
    QStandardItem* findItemByPath(QStandardItem *parent, const QString &filePath);
    void refreshAllItems();

    // 搜索用的成员变量
    QWidget *searchPopup = nullptr;
    QLineEdit *searchInput = nullptr;
    QListWidget *searchResults = nullptr;
    bool currentSearchIsKnowledgeBase = true;
    QList<QPair<QString, QString>> allKnowledgeItems; // 知识点名称, 文件路径
    QList<QPair<QString, QString>> allErrorItems;     // 同上

    QWidget *statsPopup = nullptr;
    QListWidget *statsResults = nullptr;
    bool statsVisible = false;

    // 知识点选择面板
    QWidget *knowledgeSelectPopup = nullptr;
    QListWidget *knowledgeSelectList = nullptr;
    QStringList selectedKnowledges;        // 当前已选中的知识点
    QSet<QString> invalidKnowledges;       // 已失效的知识点
    bool knowledgeSelectVisible = false;
    QString currentErrorFilePath;          // 当前打开的错题文件路径

    QString currentKbMarkdown;
    QString currentErrorMarkdown;
};

#endif // KNOWLEDGEBOOKWIDGET_H
