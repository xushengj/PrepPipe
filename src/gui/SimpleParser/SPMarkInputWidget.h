#ifndef SPMARKINPUTWIDGET_H
#define SPMARKINPUTWIDGET_H

#include <QWidget>
#include "src/lib/Tree/SimpleParser.h"
#include "src/gui/AnonymousElementListController.h"
#include "src/gui/SimpleParser/BoundaryDeclarationEditWidget.h"

namespace Ui {
class SPMarkInputWidget;
}

class SPMarkInputWidget : public QWidget
{
    Q_OBJECT

public:
    using StorageData = SimpleParser::NamedBoundary;
    using HelperData = void;

public:
    explicit SPMarkInputWidget(QWidget *parent = nullptr);
    ~SPMarkInputWidget();

    void setNamedBoundaryListModel(QStringListModel* model) {
        namedBoundaryListModel = model;
    }

    void setData(const SimpleParser::NamedBoundary& dataArg);
    void setData(const QString& name, const SimpleParser::NamedBoundary& dataArg) {
        Q_UNUSED(name)
        setData(dataArg);
    }
    void getData(const QString& name, SimpleParser::NamedBoundary& dataArg);

    void setNamedBoundaryCheckCallback(std::function<bool(const QString&)> cb);

    BoundaryDeclarationEditWidget* createElementInputWidget(AnonymousElementListControllerObject* obj);

public slots:
    void nameUpdated(const QString& name);

signals:
    void dirty();
    void gotoMarkRequested(const QString& name);

private slots:
    void markTypeChanged();

private:
    Ui::SPMarkInputWidget *ui;
    QStringListModel* namedBoundaryListModel = nullptr;

    AnonymousElementListController<BoundaryDeclarationEditWidget> elementCtl;

    std::function<bool(const QString&)> namedBoundaryCheckCB;
};

#endif // SPMARKINPUTWIDGET_H
