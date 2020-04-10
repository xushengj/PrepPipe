#ifndef STATICOBJECTINDEXDB_H
#define STATICOBJECTINDEXDB_H

#include "src/lib/ObjectBase.h"

#include <QObject>
#include <QString>
#include <QSet>
#include <QHash>
#include <QDataStream>

// Note that this is basically just a stub at this moment

class StaticObjectIndexDB : public QObject
{
    Q_OBJECT
public:
    static StaticObjectIndexDB* inst() {
        return ptr;
    }

    static void createInstance();
    static void destructInstance();

    ObjectBase* getObject(const QStringList& nameSpace, const QString& name);

    struct NameSpaceInfo {
        QString filePath;
        QHash<QString, ObjectBase*> objects; // not saved / restored in serialization; load all on demand
    };

    struct SerializationData {
        int version;
        QHash<QStringList, QString> filePathMap;
    };

private:
    explicit StaticObjectIndexDB();
    static StaticObjectIndexDB* ptr;
    QHash<QStringList, NameSpaceInfo*> nameSpaceInfo;

signals:

};

#endif // STATICOBJECTINDEXDB_H
