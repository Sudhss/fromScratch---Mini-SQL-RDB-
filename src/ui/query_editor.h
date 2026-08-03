#pragma once

#include <QPlainTextEdit>
#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>

namespace minidb {

class SqlHighlighter;

class QueryEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit QueryEditor(QWidget *parent = nullptr);
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

signals:
    void executeRequested(const QString &sql);
    void executeSelectedRequested(const QString &sql);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    SqlHighlighter *highlighter;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(QueryEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    QueryEditor *codeEditor;
};

} // namespace minidb
