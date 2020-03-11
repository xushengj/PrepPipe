#ifndef OBJECTTREEWIDGET_H
#define OBJECTTREEWIDGET_H

#include "src/lib/ObjectContext.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <functional>

class ObjectTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    ObjectTreeWidget(QWidget* parent = nullptr)
        : QTreeWidget(parent)
    {}

    void setGetReferenceCallback(std::function<bool(QTreeWidgetItem*, ObjectContext::AnonymousObjectReference&)> getRefCB) {
        getRef = getRefCB;
    }
    void addSelfContextIndex(int index) {
        selfContextIndexList.push_back(index);
    }

    virtual ~ObjectTreeWidget(){}

signals:
    void objectDropped(const QList<ObjectBase*>& vec);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void dragMoveEvent(QDragMoveEvent* event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent* event) override;

public:
    static QList<ObjectContext::AnonymousObjectReference> recoverReference(const QMimeData* data);
    static QList<ObjectBase*> solveReference(const QList<ObjectContext::AnonymousObjectReference>& refList);

    static QList<ObjectBase*> solveReference(const QMimeData* data) {
        return solveReference(recoverReference(data));
    }

private:
    bool isValidExternReference(const QMimeData* data, QList<ObjectContext::AnonymousObjectReference>& refList);

protected:
    virtual QMimeData * mimeData(const QList<QTreeWidgetItem *> items) const override;

private:
    std::function<bool(QTreeWidgetItem*, ObjectContext::AnonymousObjectReference&)> getRef;
    QList<int> selfContextIndexList;
};

#endif // OBJECTTREEWIDGET_H
