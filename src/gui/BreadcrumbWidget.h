#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include <QWidget>
#include <QString>

#include <functional>

class BreadcrumbWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BreadcrumbWidget(QWidget *parent = nullptr);
    void installParentCallback(std::function<int(int)> parentCB);

signals:
    void indexChanged(int index);

public slots:
    void changeIndex(int index);
    void treeUpdated(int numNodes);

private:
    std::function<int(int)> callback_getParent;
    std::function<QString(int)> callback_getDescription;
};

#endif // BREADCRUMBWIDGET_H
