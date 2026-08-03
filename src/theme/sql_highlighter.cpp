#include "theme/sql_highlighter.h"
#include "ui/theme.h"

namespace minidb {

SqlHighlighter::SqlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupRules();
}

void SqlHighlighter::setupRules() {
    HighlightingRule rule;

    // Keywords
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(Theme::SYN_KEYWORD));
    keywordFormat.setFontWeight(QFont::Bold);
    
    QStringList keywordPatterns = {
        "\\bSELECT\\b", "\\bFROM\\b", "\\bWHERE\\b", "\\bINSERT\\b", "\\bUPDATE\\b",
        "\\bDELETE\\b", "\\bCREATE\\b", "\\bDROP\\b", "\\bTABLE\\b", "\\bINTO\\b",
        "\\bVALUES\\b", "\\bSET\\b", "\\bORDER\\b", "\\bBY\\b", "\\bGROUP\\b",
        "\\bHAVING\\b", "\\bLIMIT\\b", "\\bJOIN\\b", "\\bINNER\\b", "\\bLEFT\\b",
        "\\bRIGHT\\b", "\\bON\\b", "\\bAND\\b", "\\bOR\\b", "\\bNOT\\b", "\\bIN\\b",
        "\\bBETWEEN\\b", "\\bLIKE\\b", "\\bIS\\b", "\\bAS\\b", "\\bDISTINCT\\b",
        "\\bALTER\\b", "\\bADD\\b", "\\bCOLUMN\\b", "\\bSHOW\\b", "\\bTABLES\\b",
        "\\bDESCRIBE\\b", "\\bASC\\b", "\\bDESC\\b", "\\bPRIMARY\\b", "\\bKEY\\b",
        "\\bDEFAULT\\b", "\\bUNIQUE\\b", "\\bEXISTS\\b", "\\bCROSS\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Data types
    QTextCharFormat typeFormat;
    typeFormat.setForeground(QColor(Theme::SYN_TYPE));
    
    QStringList typePatterns = {
        "\\bINT\\b", "\\bINTEGER\\b", "\\bFLOAT\\b", "\\bDOUBLE\\b", "\\bREAL\\b",
        "\\bVARCHAR\\b", "\\bTEXT\\b", "\\bSTRING\\b", "\\bBOOL\\b", "\\bBOOLEAN\\b",
        "\\bDATE\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = typeFormat;
        highlightingRules.append(rule);
    }

    // Functions
    QTextCharFormat functionFormat;
    functionFormat.setForeground(QColor(Theme::SYN_FUNCTION));
    functionFormat.setFontWeight(QFont::Bold);
    
    QStringList functionPatterns = {
        "\\bCOUNT\\b", "\\bSUM\\b", "\\bAVG\\b", "\\bMIN\\b", "\\bMAX\\b"
    };

    for (const QString &pattern : functionPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = functionFormat;
        highlightingRules.append(rule);
    }

    // Numbers
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(Theme::SYN_NUMBER));
    rule.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // NULL, TRUE, FALSE
    QTextCharFormat nullFormat;
    nullFormat.setForeground(QColor(Theme::SYN_NULL));
    nullFormat.setFontWeight(QFont::Bold);
    nullFormat.setFontItalic(true);
    
    QStringList nullPatterns = { "\\bNULL\\b", "\\bTRUE\\b", "\\bFALSE\\b" };
    for (const QString &pattern : nullPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = nullFormat;
        highlightingRules.append(rule);
    }

    // Operators
    QTextCharFormat operatorFormat;
    operatorFormat.setForeground(QColor(Theme::SYN_OPERATOR));
    rule.pattern = QRegularExpression("[=<>!\\+\\-\\*\\/%]");
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Strings
    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(Theme::SYN_STRING));
    rule.pattern = QRegularExpression("'[^']*'");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    // Single line comments
    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(QColor(Theme::SYN_COMMENT));
    singleLineCommentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // Multi-line comments
    multiLineCommentFormat.setForeground(QColor(Theme::SYN_COMMENT));
    multiLineCommentFormat.setFontItalic(true);
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void SqlHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle block comments
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

} // namespace minidb
