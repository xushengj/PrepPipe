#include "AnonymousElementListController.h"

#include <QDebug>

AnonymousElementListControllerObject::AnonymousElementListControllerObject()
    : QObject(nullptr)
{

}

void AnonymousElementListControllerObject::init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg)
{
    listWidget = listWidgetArg;
    stackedWidget = stackedWidgetArg;
    connect(listWidget, &QListWidget::currentRowChanged, this, &AnonymousElementListControllerObject::currentElementChangeHandler);
}

void AnonymousElementListControllerObject::currentElementChangeHandler(int index)
{
    if (currentElementChangeCB) {
        currentElementChangeCB(index);
    } else {
        qWarning() << "Current element changed but no callback is installed; change event ignored";
    }
}


