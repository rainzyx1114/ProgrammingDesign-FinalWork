#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QString>

class MarkdownParser
{
public:
    static QString toHtml(const QString &markdown, const QString &baseDir);

private:
    static QString processInline(const QString &text, const QString &baseDir);
};

#endif // MARKDOWNPARSER_H
