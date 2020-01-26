#include "BreadcrumbWidget.h"

BreadcrumbWidget::BreadcrumbWidget(QWidget *parent) : QWidget(parent)
{

}

void BreadcrumbWidget::changeIndex(int index)
{
    Q_UNUSED(index)
}

void BreadcrumbWidget::treeUpdated(int numNodes)
{
    Q_UNUSED(numNodes)
}
