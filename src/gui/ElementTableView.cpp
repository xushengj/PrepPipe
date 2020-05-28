#include "ElementTableView.h"

ElementTableView::ElementTableView(QWidget* parent)
    : QTableView(parent)
{
    setMouseTracking(true);
}

void ElementTableView::resetHoverData()
{
    itemHovered = QModelIndex();
}

void ElementTableView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint position = event->pos();
    QModelIndex index = indexAt(position);
    if (itemHovered.isValid() && itemHovered != index) {
        emit itemHoverFinished(itemHovered);
    }
    if (index.isValid() && itemHovered != index) {
        emit itemHoverStarted(index);
    }
    itemHovered = index;
    QTableView::mouseMoveEvent(event);
}
