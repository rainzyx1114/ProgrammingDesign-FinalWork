#include "markdownparser.h"
#include <QRegularExpression>
#include <QStringList>
#include <QFileInfo>
#include <QStringView>
#include <utility>

QString MarkdownParser::processInline(const QString &text, const QString &baseDir)
{
    QString processed = text;

    // 图片
    QString imageProcessed;
    int lastPos = 0;
    QRegularExpression imageRe("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)");
    QRegularExpressionMatchIterator it = imageRe.globalMatch(processed);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        imageProcessed += QStringView(processed).mid(lastPos, match.capturedStart() - lastPos);
        QString alt = match.captured(1);
        QString imgPath = match.captured(2);
        QString fileName = QFileInfo(imgPath).fileName();
        QString fullPath = baseDir + "/" + fileName;
        if (QFile::exists(fullPath)) {
            imageProcessed += "<img src='" + fullPath + "' alt='" + alt + "' style='max-width:100%;'>";
        } else {
            imageProcessed += "<span style='color:red;'>[图片未找到: " + fileName + "]</span>";
        }
        lastPos = match.capturedEnd();
    }
    imageProcessed += QStringView(processed).mid(lastPos);
    processed = imageProcessed;

    // 行内代码
    QRegularExpression inlineCodeRe("`([^`]+)`");
    processed.replace(inlineCodeRe, "<code style='background-color:#F0F0F0; padding:0 2px; border-radius:2px; font-family:monospace;'>\\1</code>");

    // 粗体
    QRegularExpression boldRe("\\*\\*(.+?)\\*\\*");
    processed.replace(boldRe, "<b>\\1</b>");

    // 斜体
    QRegularExpression italicRe("\\*(.+?)\\*");
    processed.replace(italicRe, "<i>\\1</i>");

    // 删除线
    QRegularExpression strikeRe("~~(.+?)~~");
    processed.replace(strikeRe, "<s>\\1</s>");

    return processed;
}

QString MarkdownParser::toHtml(const QString &markdown, const QString &baseDir)
{
    QString html;
    html += "<html><body style='font-size:14px; line-height:0.8; padding-left:8px;'>";

    QStringList lines = markdown.split("\n");
    bool inCodeBlock = false;
    QString codeBlockContent;
    bool inUnorderedList = false;
    bool inOrderedList = false;

    for (int i = 0; i < lines.size(); i++) {
        const QString &line = lines[i];
        QString trimmed = line.trimmed();

        // 代码块
        if (trimmed.startsWith("```")) {
            if (inUnorderedList) { html += "</ul>"; inUnorderedList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            if (!inCodeBlock) {
                inCodeBlock = true;
                codeBlockContent.clear();
            } else {
                inCodeBlock = false;
                html += "<pre style='background-color:#F0F0F0; padding:4px 8px; margin:0; border-radius:4px; font-family:monospace;'>"
                        + codeBlockContent.toHtmlEscaped() + "</pre>";
            }
            continue;
        }
        if (inCodeBlock) {
            if (!codeBlockContent.isEmpty()) codeBlockContent += "\n";
            codeBlockContent += line;
            continue;
        }

        // 水平线
        if (trimmed == "---" || trimmed == "***" || trimmed == "___") {
            if (inUnorderedList) { html += "</ul>"; inUnorderedList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<hr style='border:0; border-top:1px solid #CCC; margin:0;'>";
            continue;
        }

        // 表格：以 | 开头和结尾
        if (trimmed.startsWith("|") && trimmed.endsWith("|")) {
            if (inUnorderedList) { html += "</ul>"; inUnorderedList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            QStringList cells = trimmed.split("|");
            // 去掉首尾空元素
            if (cells.size() > 1 && cells.first().trimmed().isEmpty()) cells.removeFirst();
            if (cells.size() > 1 && cells.last().trimmed().isEmpty()) cells.removeLast();

            bool isHeaderRow = false;
            if (i + 1 < lines.size()) {
                QString nextTrimmed = lines[i + 1].trimmed();
                if (nextTrimmed.startsWith("|") && nextTrimmed.contains("---")) {
                    isHeaderRow = true;
                }
            }

            QString rowTag = isHeaderRow ? "th" : "td";
            QString style = isHeaderRow ? "style='background-color:#E0E0E0; font-weight:bold; padding:4px 8px; border:1px solid #CCC;'"
                                        : "style='padding:4px 8px; border:1px solid #CCC;'";
            html += "<tr>";
            for (const QString &cell : std::as_const(cells)) {
                QString processed = cell.trimmed();
                // 行内代码
                QRegularExpression inlineCodeRe("`([^`]+)`");
                processed.replace(inlineCodeRe, "<code style='background-color:#F0F0F0; padding:1px 4px; border-radius:2px; font-family:monospace;'>\\1</code>");
                // 粗体
                QRegularExpression boldRe("\\*\\*(.+?)\\*\\*");
                processed.replace(boldRe, "<b>\\1</b>");
                // 斜体
                QRegularExpression italicRe("\\*(.+?)\\*");
                processed.replace(italicRe, "<i>\\1</i>");
                // 删除线
                QRegularExpression strikeRe("~~(.+?)~~");
                processed.replace(strikeRe, "<s>\\1</s>");
                html += "<" + rowTag + " " + style + ">" + processed + "</" + rowTag + ">";
            }
            html += "</tr>";

            // 跳过分隔行
            if (isHeaderRow) {
                i++;
            }
            continue;
        }

        // 无序列表
        if (trimmed.startsWith("- ") || trimmed.startsWith("* ")) {
            if (!inUnorderedList) {
                html += "<ul style='list-style-type:disc; list-style-position:inside; padding-left:0; margin-left:0;'>";
                inUnorderedList = true;
            }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            QString content = trimmed.mid(2);
            content = processInline(content, baseDir);
            html += "<li>" + content + "</li>";
            continue;
        } else if (inUnorderedList) {
            html += "</ul>";
            inUnorderedList = false;
        }

        // 有序列表
        QRegularExpression orderedRe("^(\\d+)\\.\\s+(.+)");
        QRegularExpressionMatch orderedMatch = orderedRe.match(trimmed);
        if (orderedMatch.hasMatch()) {
            if (!inOrderedList) {
                html += "<ol style='list-style-position:inside; padding-left:0; margin-left:0;'>";
                inOrderedList = true;
            }
            if (inUnorderedList) { html += "</ul>"; inUnorderedList = false; }
            QString content = orderedMatch.captured(2);
            content = processInline(content, baseDir);
            html += "<li>" + content + "</li>";
            continue;
        } else if (inOrderedList) {
            html += "</ol>";
            inOrderedList = false;
        }

        // 引用
        if (trimmed.startsWith("> ")) {
            QString content = trimmed.mid(2);
            content = processInline(content, baseDir);
            html += "<blockquote style='border-left:3px solid #CCC; padding-left:4px; margin:0; color:#666;'>"
                    + content + "</blockquote>";
            continue;
        }

        // 标题
        if (trimmed.startsWith("###### ")) {
            QString content = trimmed.mid(7).toHtmlEscaped();
            html += "<h6 style='font-size:14px; font-weight:bold; margin:0;'>" + content + "</h6>";
            continue;
        }
        if (trimmed.startsWith("##### ")) {
            QString content = trimmed.mid(6).toHtmlEscaped();
            html += "<h5 style='font-size:15px; font-weight:bold; margin:0;'>" + content + "</h5>";
            continue;
        }
        if (trimmed.startsWith("#### ")) {
            QString content = trimmed.mid(5).toHtmlEscaped();
            html += "<h4 style='font-size:16px; font-weight:bold; margin:0;'>" + content + "</h4>";
            continue;
        }
        if (trimmed.startsWith("### ")) {
            QString content = trimmed.mid(4).toHtmlEscaped();
            html += "<h3 style='font-size:18px; font-weight:bold; margin:0;'>" + content + "</h3>";
            continue;
        }
        if (trimmed.startsWith("## ")) {
            QString content = trimmed.mid(3).toHtmlEscaped();
            html += "<h2 style='font-size:20px; font-weight:bold; margin:0;'>" + content + "</h2>";
            continue;
        }
        if (trimmed.startsWith("# ")) {
            QString content = trimmed.mid(2).toHtmlEscaped();
            html += "<h1 style='font-size:22px; font-weight:bold; margin:0;'>" + content + "</h1>";
            continue;
        }

        // 空行
        if (trimmed.isEmpty()) {
            html += "<br>";
            continue;
        }

        // 普通段落
        QString processed = processInline(trimmed, baseDir);
        html += "<p style='margin:0;'>" + processed + "</p>";
    }

    // 收尾
    if (inCodeBlock) {
        html += "<pre style='background-color:#F0F0F0; padding:8px; border-radius:4px; font-family:monospace;'>"
                + codeBlockContent.toHtmlEscaped() + "</pre>";
    }
    if (inUnorderedList) html += "</ul>";
    if (inOrderedList) html += "</ol>";

    html += "</body></html>";
    return html;
}