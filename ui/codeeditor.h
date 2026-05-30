#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QTextBlock>
#include<QApplication>
class CodeEditor;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

private:
    CodeEditor *codeEditor;
};

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int calculateLineNumberWidth() const;

    // 设置当前执行行和下一行
    void setCurrentLine(int line);
    void setNextLine(int line);
    void clearExecutionLines();
    void updateLineNumberAreaWidth(int newBlockCount);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    int m_currentLine = -1;  // 当前执行行（红色箭头），1-based
    int m_nextLine = -1;     // 下一条要执行的行（绿色箭头），1-based
};

#endif // CODEEDITOR_H
