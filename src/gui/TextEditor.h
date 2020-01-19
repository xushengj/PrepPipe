#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>

// reference: https://doc.qt.io/qt-5/qtwidgets-widgets-codeeditor-example.html

class TextLineNumberArea;

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

    friend class TextLineNumberArea;
public:
    TextEditor(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);
    void highlightCurrentLine();

private:
    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

private:
    TextLineNumberArea* lineNumberArea;
};

class TextLineNumberArea : public QWidget
{
    Q_OBJECT

public:
    TextLineNumberArea(TextEditor* _editor) : QWidget(_editor), editor(_editor) {}

    QSize sizeHint() const override {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        editor->lineNumberAreaPaintEvent(event);
    }

private:
    TextEditor* editor;
};

#endif // TEXTEDITOR_H
