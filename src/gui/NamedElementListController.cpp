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
}

void NamedElementListControllerObject::currentElementChangeHandler(int index)
{
    if (currentElementChangeCB) {
        currentElementChangeCB(index);
    } else {
        qWarning() << "Current element changed but no callback is installed; change event ignored";
    }
}

void NamedElementListControllerObject::tryGoToElement(const QString& elementName)
{
    if (gotoElementCB) {
        gotoElementCB(elementName);
    }
}
