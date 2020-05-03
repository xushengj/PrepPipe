#include "ElementListWidget.h"

ElementListWidget::ElementListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setMouseTracking(true);
}

void ElementListWidget::resetHoverData()
{
    itemHovered = nullptr;
}

void ElementListWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint position = event->pos();
    QListWidgetItem* item = itemAt(position);
    if (itemHovered && itemHovered != item) {
        emit itemHoverFinished(itemHovered);
    }
    if (item && itemHovered != item) {
        emit itemHoverStarted(item);
    }
    itemHovered = item;
    QListWidget::mouseMoveEvent(event);
}
