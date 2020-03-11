#include "DropTestLabel.h"

#include <QDragEnterEvent>
#include <QDropEvent>

DropTestLabel::DropTestLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setAcceptDrops(true);
}

DropTestLabel::DropTestLabel(QWidget *parent)
    : QLabel(parent)
{
    setAcceptDrops(true);
}

DropTestLabel::~DropTestLabel()
{

}

void DropTestLabel::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void DropTestLabel::dropEvent(QDropEvent* event)
{
    const QMimeData* ptr = event->mimeData();
    emit dataDropped(ptr);
    event->acceptProposedAction();
}

