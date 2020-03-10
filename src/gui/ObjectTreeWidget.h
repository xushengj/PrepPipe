#ifndef OBJECTTREEWIDGET_H
#define OBJECTTREEWIDGET_H

#include "src/lib/ObjectContext.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <functional>

class ObjectTreeWidget : public QTreeWidget {
public:
    ObjectTreeWidget(QWidget* parent = nullptr)
        : QTreeWidget(parent)
    {}

    void setGetReferenceCallback(std::function<bool(QTreeWidgetItem*, ObjectContext::AnonymousObjectReference&)> getRefCB) {
        getRef = getRefCB;
    }

    virtual ~ObjectTreeWidget(){}
public:
    static QList<ObjectBase*> solveReference(const QMimeData* data);
    static QList<ObjectContext::AnonymousObjectReference> recoverReference(const QMimeData* data);
    static bool isValidReference(const QMimeData* data);

protected:
    virtual QMimeData * mimeData(const QList<QTreeWidgetItem *> items) const override;

private:
    std::function<bool(QTreeWidgetItem*, ObjectContext::AnonymousObjectReference&)> getRef;
};

#endif // OBJECTTREEWIDGET_H
