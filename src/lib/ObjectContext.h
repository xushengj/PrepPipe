#ifndef OBJECTCONTEXT_H
#define OBJECTCONTEXT_H

#include "src/lib/ObjectBase.h"

#include <QObject>

/**
 * @brief The ObjectContext class manages a collection of objects
 */
class ObjectContext : public QObject
{
    Q_OBJECT

public:
    ObjectContext() = default;
    ObjectContext(const QString& rootDirectory);

    ~ObjectContext() {
        clear();
    }

    void loadAllObjectsFromDirectory(const QString& dir, const QStringList& nameSpace = QStringList() );

    int getNumObjects() const {
        return objects.size();
    }
    void clear();

    ObjectBase* getObject(const QString& name, const QStringList& nameSpace = QStringList()) const;

private:
    QString rootDirectory;
    QStringList baseNameSpace;
    QList<ObjectBase*> objects;
    QHash<QStringList,QHash<QString,int>> objectIndexByNameSpace;

public:
    decltype(objects.begin())   begin() {return objects.begin();}
    decltype(objects.end())     end()   {return objects.end();}
};

#endif // OBJECTCONTEXT_H
