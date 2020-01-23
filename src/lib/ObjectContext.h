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
    ObjectContext(const QString& mainDirectory);

    ~ObjectContext() {
        clear();
    }

    void swap(ObjectContext& rhs) {
        mainDirectory.swap(rhs.mainDirectory);
        objects.swap(rhs.objects);
        objectMap.swap(rhs.objectMap);
    }

    int getNumObjects() const {
        return objects.size();
    }
    void clear();

    ObjectBase* getObject(const QString& name) const;

    QString getDirectory() const {
        return mainDirectory;
    }

    void addObject(ObjectBase* obj);
    void releaseObject(ObjectBase* obj);

    void setDirectory(const QString& newDirectory);

private:
    void loadAllObjectsFromDirectory();

private:
    QString mainDirectory;
    QList<ObjectBase*> objects;
    QHash<QString, ObjectBase*> objectMap;

public:
    decltype(objects.begin())   begin() {return objects.begin();}
    decltype(objects.end())     end()   {return objects.end();}

    decltype(objects.size())    size()  const {return objects.size();}
};

#endif // OBJECTCONTEXT_H
