#ifndef GENERALTREEEDITOR_H
#define GENERALTREEEDITOR_H

#include <QWidget>

namespace Ui {
class GeneralTreeEditor;
}

class GeneralTreeEditor : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralTreeEditor(QWidget *parent = nullptr);
    ~GeneralTreeEditor();

private:
    Ui::GeneralTreeEditor *ui;
};

#endif // GENERALTREEEDITOR_H
