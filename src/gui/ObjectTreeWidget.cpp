#include "src/gui/ObjectTreeWidget.h"

#include <QMimeData>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>

namespace {
const QString MIME_TYPE = QStringLiteral("application/supp-object-ref");
const QString REF_CTX_ID = QStringLiteral("ContextID");
const QString REF_REF_ID = QStringLiteral("ReferenceID");
}

QMimeData* ObjectTreeWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{

    QJsonArray itemArray;
    for (QTreeWidgetItem* item : items) {
        ObjectContext::AnonymousObjectReference ref;
        if (getRef(item, ref)) {
            QJsonObject refObj;
            refObj.insert(REF_CTX_ID, ref.ctxIndex);
            refObj.insert(REF_REF_ID, ref.refIndex);
            itemArray.push_back(refObj);
        }
    }

    QJsonDocument doc(itemArray);
    QByteArray ba = doc.toJson(QJsonDocument::Compact);

    QMimeData* data = new QMimeData;
    data->setData(MIME_TYPE, ba);
    return data;
}

bool ObjectTreeWidget::isValidExternReference(const QMimeData* data, QList<ObjectContext::AnonymousObjectReference>& refList)
{
    refList = recoverReference(data);
    if (refList.isEmpty()) {
        return false;
    }

    // check if there are some reference from contexts within this tree
    // we do not want internal move here (which would be duplicating objects)
    for (const auto& ref : refList) {
        if (selfContextIndexList.contains(ref.ctxIndex)) {
            return false;
        }
    }

    return true;
}

QList<ObjectBase*> ObjectTreeWidget::solveReference(const QList<ObjectContext::AnonymousObjectReference>& refList)
{
    QList<ObjectBase*> result;
    if (refList.isEmpty())
        return result;

    for (const auto& ref : refList) {
        ObjectBase* ptr = ObjectContext::resolveAnonymousReference(ref);
        if (Q_UNLIKELY(!ptr))
            return QList<ObjectBase*>();
        result.push_back(ptr);
    }

    return result;
}

QList<ObjectContext::AnonymousObjectReference> ObjectTreeWidget::recoverReference(const QMimeData* data)
{
    QList<ObjectContext::AnonymousObjectReference> result;
    QByteArray ba = data->data(MIME_TYPE);
    QJsonDocument doc = QJsonDocument::fromJson(ba);
    QJsonArray itemArray = doc.array();
    for (auto item : itemArray) {
        QJsonObject obj = item.toObject();
        int ctxIndex = obj.value(REF_CTX_ID).toInt(-1);
        int refIndex = obj.value(REF_REF_ID).toInt(-1);
        if (Q_UNLIKELY(ctxIndex == -1 || refIndex == -1))
            return QList<ObjectContext::AnonymousObjectReference>();
        ObjectContext::AnonymousObjectReference curRef;
        curRef.ctxIndex = ctxIndex;
        curRef.refIndex = refIndex;
        result.push_back(curRef);
    }
    return result;
}

void ObjectTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    QList<ObjectContext::AnonymousObjectReference> refList;
    if (isValidExternReference(event->mimeData(), refList)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ObjectTreeWidget::dropEvent(QDropEvent* event)
{
    QList<ObjectContext::AnonymousObjectReference> refList;
    if (isValidExternReference(event->mimeData(), refList)) {
        QList<ObjectBase*> objList = solveReference(refList);
        if (!objList.isEmpty()) {
            emit objectDropped(objList);
        }
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ObjectTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    event->accept();
}

void ObjectTreeWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}
