#ifndef SPMARKINPUTWIDGET_H
#define SPMARKINPUTWIDGET_H

#include <QWidget>

namespace Ui {
class SPMarkInputWidget;
}

class SPMarkInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SPMarkInputWidget(QWidget *parent = nullptr);
    ~SPMarkInputWidget();

private:
    Ui::SPMarkInputWidget *ui;
};

#endif // SPMARKINPUTWIDGET_H
