#include "ui/query_editor.h"
#include "theme/sql_highlighter.h"
#include "ui/theme.h"
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QGuiApplication>

namespace minidb {

QueryEditor::QueryEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    highlighter = new SqlHighlighter(document());

    connect(this, &QueryEditor::blockCountChanged, this, &QueryEditor::updateLineNumberAreaWidth);
    connect(this, &QueryEditor::updateRequest, this, &QueryEditor::updateLineNumberArea);
    connect(this, &QueryEditor::cursorPositionChanged, this, &QueryEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    QFont font("Cascadia Code", 11);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    // Set tab width to 4 spaces
    const int tabStop = 4 * fontMetrics().horizontalAdvance(' ');
    setTabStopDistance(tabStop);

    setStyleSheet(QString(
        "QPlainTextEdit {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "}"
    ).arg(Theme::BG_EDITOR).arg(Theme::FG_PRIMARY));
}

int QueryEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void QueryEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void QueryEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void QueryEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void QueryEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Theme::BG_HOVER);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void QueryEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(Theme::BG_SIDEBAR));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(Theme::FG_MUTED));
            painter.drawText(0, top, lineNumberArea->width() - 2, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void QueryEditor::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (e->modifiers() == Qt::ControlModifier) {
            emit executeRequested(toPlainText());
            return;
        } else if (e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            QString selected = textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
            if (!selected.isEmpty()) {
                emit executeSelectedRequested(selected);
            }
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(e);
}

} // namespace minidb
