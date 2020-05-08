#include "src/gui/NamedElementListController.h"
#include <QDebug>

NamedElementListControllerObject::NamedElementListControllerObject()
    : QObject(nullptr)
{}

void NamedElementListControllerObject::init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg)
{
    listWidget = listWidgetArg;
    stackedWidget = stackedWidgetArg;
    connect(listWidget, &QListWidget::currentRowChanged, this, &NamedElementListControllerObject::currentElementChangeHandler);

    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QWidget::customContextMenuRequested, this, &NamedElementListControllerObject::listWidgetContextMenuEventHandler);
}

void NamedElementListControllerObject::currentElementChangeHandler(int index)
{
    if (currentElementChangeCB) {
        currentElementChangeCB(index);
    } else {
        qWarning() << "Current element changed but no callback is installed; change event ignored";
    }
}

void NamedElementListControllerObject::listWidgetContextMenuEventHandler(const QPoint& p)
{
    if (listWidgetContextMenuEventCB) {
        listWidgetContextMenuEventCB(p);
    } else {
        qWarning() << "List widget context menu requested but no callback is installed; request ignored";
    }
}

void NamedElementListControllerObject::tryGoToElement(const QString& elementName)
{
    if (gotoElementCB) {
        gotoElementCB(elementName);
    }
}

void NamedElementListControllerObject::tryCreateElement(const QString& elementName)
{
    if (createElementCB) {
        createElementCB(elementName);
    } else {
        qWarning() << "Element creation requested but no callback installed; request ignored";
    }
}
