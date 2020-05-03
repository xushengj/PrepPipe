#ifndef SPCONTENTINPUTWIDGET_H
#define SPCONTENTINPUTWIDGET_H

#include <QWidget>

namespace Ui {
class SPContentInputWidget;
}

class SPContentInputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SPContentInputWidget(QWidget *parent = nullptr);
    ~SPContentInputWidget();

private:
    Ui::SPContentInputWidget *ui;
};

#endif // SPCONTENTINPUTWIDGET_H
