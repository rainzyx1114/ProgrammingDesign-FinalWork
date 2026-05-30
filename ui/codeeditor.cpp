#include "codeeditor.h"
#include <QPainter>
#include <QTextFormat>
#include <QScrollBar>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QApplication>

LineNumberArea::LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->calculateLineNumberWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::wheelEvent(QWheelEvent *event)
{
    // 将滚轮事件转发给代码编辑器，保证滚动同步
    QApplication::sendEvent(codeEditor->viewport(), event);
}

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), m_currentLine(-1), m_nextLine(-1)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::calculateLineNumberWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // 额外留出箭头图标的空间（约20像素）+ 行号数字空间 + 左右边距
    int arrowSpace = 20;
    int numberSpace = fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    int padding = 8;

    return arrowSpace + numberSpace + padding;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(calculateLineNumberWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), calculateLineNumberWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    // 不再高亮光标所在行，避免与执行箭头混淆
    // 如果需要光标高亮可以取消下面的注释
    /*
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
    */
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1E1E1E")); // 与编辑器背景一致

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    // 计算可见区域的顶部偏移
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingRect(block).height();

    int arrowWidth = 20; // 箭头区域宽度

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNumber = blockNumber + 1; // 1-based 行号

            // 绘制行号
            painter.setPen(QColor("#858585")); // VS Code 风格行号颜色
            QString number = QString::number(lineNumber);
            painter.setFont(font());
            painter.drawText(QRect(arrowWidth, static_cast<int>(top),
                                   lineNumberArea->width() - arrowWidth - 4,
                                   static_cast<int>(bottom - top)),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // 绘制红色箭头（当前执行行）
            if (m_currentLine > 0 && lineNumber == m_currentLine) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#FF4444"));

                int centerY = static_cast<int>(top + (bottom - top) / 2.0);
                int arrowX = 4;
                int arrowSize = 10;

                QPolygon arrow;
                arrow << QPoint(arrowX, centerY - arrowSize / 2)
                      << QPoint(arrowX + arrowSize, centerY)
                      << QPoint(arrowX, centerY + arrowSize / 2);
                painter.drawPolygon(arrow);
            }

            // 绘制绿色箭头（下一条要执行的行）
            if (m_nextLine > 0 && lineNumber == m_nextLine) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#44CC44"));

                int centerY = static_cast<int>(top + (bottom - top) / 2.0);
                int arrowX = 4;
                int arrowSize = 10;

                QPolygon arrow;
                arrow << QPoint(arrowX, centerY - arrowSize / 2)
                      << QPoint(arrowX + arrowSize, centerY)
                      << QPoint(arrowX, centerY + arrowSize / 2);
                painter.drawPolygon(arrow);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::setCurrentLine(int line)
{
    m_currentLine = line;
    lineNumberArea->update();

    // 高亮当前执行行背景
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (m_currentLine > 0) {
        QTextBlock block = document()->findBlockByNumber(m_currentLine - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            cursor.select(QTextCursor::LineUnderCursor);

            QTextEdit::ExtraSelection currentSel;
            currentSel.cursor = cursor;
            currentSel.format.setBackground(QColor(255, 80, 80, 40)); // 淡红色背景
            currentSel.format.setProperty(QTextFormat::FullWidthSelection, true);
            extraSelections.append(currentSel);
        }
    }

    if (m_nextLine > 0) {
        QTextBlock block = document()->findBlockByNumber(m_nextLine - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            cursor.select(QTextCursor::LineUnderCursor);

            QTextEdit::ExtraSelection nextSel;
            nextSel.cursor = cursor;
            nextSel.format.setBackground(QColor(80, 200, 80, 40)); // 淡绿色背景
            nextSel.format.setProperty(QTextFormat::FullWidthSelection, true);
            extraSelections.append(nextSel);
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::setNextLine(int line)
{
    m_nextLine = line;
    // 重新应用高亮
    setCurrentLine(m_currentLine);
}

void CodeEditor::clearExecutionLines()
{
    m_currentLine = -1;
    m_nextLine = -1;
    lineNumberArea->update();
    setExtraSelections(QList<QTextEdit::ExtraSelection>());
}
