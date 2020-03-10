#include "src/gui/ObjectTreeWidget.h"

#include <QMimeData>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

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

bool ObjectTreeWidget::isValidReference(const QMimeData* data)
{
    return data->hasFormat(MIME_TYPE);
}

QList<ObjectBase*> ObjectTreeWidget::solveReference(const QMimeData* data)
{
    QList<ObjectBase*> result;
    QList<ObjectContext::AnonymousObjectReference> refList = recoverReference(data);
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
