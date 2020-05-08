#ifndef SPCONTENTINPUTWIDGET_H
#define SPCONTENTINPUTWIDGET_H

#include <QWidget>
#include "src/lib/Tree/SimpleParser.h"

namespace Ui {
class SPContentInputWidget;
}

class SPContentInputWidget : public QWidget
{
    Q_OBJECT

public:
    using StorageData = SimpleParser::ContentType;
    using HelperData = void;

public:
    explicit SPContentInputWidget(QWidget *parent = nullptr);
    ~SPContentInputWidget();

    void setData(const SimpleParser::ContentType& dataArg);
    void setData(const QString& name, const SimpleParser::ContentType& dataArg) {
        Q_UNUSED(name)
        setData(dataArg);
    }
    void getData(const QString& name, SimpleParser::ContentType& dataArg);

public slots:
    void nameUpdated(const QString& name);

signals:
    void dirty();

private:
    Ui::SPContentInputWidget *ui;
};

#endif // SPCONTENTINPUTWIDGET_H
