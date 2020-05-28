#ifndef ELEMENTTABLEVIEW_H
#define ELEMENTTABLEVIEW_H

#include <QTableView>
#include <QMouseEvent>

class ElementTableView : public QTableView
{
    Q_OBJECT
public:
    ElementTableView(QWidget* parent = nullptr);

signals:
    void itemHoverStarted(const QModelIndex& item);
    void itemHoverFinished(const QModelIndex& item);

public slots:
    void resetHoverData(); // call this if the list is modified

protected:
    void mouseMoveEvent(QMouseEvent *event);

private:
    QModelIndex itemHovered;
};

#endif // ELEMENTTABLEVIEW_H
