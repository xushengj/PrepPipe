#ifndef ELEMENTLISTWIDGET_H
#define ELEMENTLISTWIDGET_H

#include <QListWidget>
#include <QMouseEvent>

class ElementListWidget : public QListWidget
{
    Q_OBJECT
public:
    ElementListWidget(QWidget* parent = nullptr);

signals:
    void itemHoverStarted(QListWidgetItem* item);
    void itemHoverFinished(QListWidgetItem* item);

public slots:
    void resetHoverData(); // call this if the list is modified

protected:
    void mouseMoveEvent(QMouseEvent *event);

private:
    QListWidgetItem* itemHovered = nullptr;
};

#endif // ELEMENTLISTWIDGET_H
