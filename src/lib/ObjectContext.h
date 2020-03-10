#ifndef OBJECTCONTEXT_H
#define OBJECTCONTEXT_H

#include "src/lib/ObjectBase.h"

#include <QObject>
#include <QHash>

/**
 * @brief The ObjectContext class manages a collection of objects
 */
class ObjectContext : public QObject
{
    Q_OBJECT

public:
    // make sure the default constructed one is invalid
    struct AnonymousObjectReference {
        int ctxIndex = -1;
        int refIndex = -1;

        bool isValid() const {return ctxIndex >= 0 && refIndex >= 0;}
    };
public:
    ObjectContext();
    ObjectContext(const QString& mainDirectory);
    ObjectContext(const ObjectContext& src) = delete;
    ObjectContext(ObjectContext&& src) = delete;

    ~ObjectContext();

    void swap(ObjectContext& rhs) {
        mainDirectory.swap(rhs.mainDirectory);
        objects.swap(rhs.objects);
        objectMap.swap(rhs.objectMap);
    }

    int getNumObjects() const {
        return objects.size();
    }
    void clear();

    ObjectBase* getObject(const QString& name) const {
        return objectMap.value(name, nullptr);
    }

    QString getDirectory() const {
        return mainDirectory;
    }

    void addObject(ObjectBase* obj);
    void releaseObject(ObjectBase* obj);
    int getObjectReference(ObjectBase* obj);

    void setDirectory(const QString& newDirectory);

    int getContextIndex() const {return ctxIndex;}

    static ObjectBase* resolveNamedReference(const ObjectBase::NamedReference& ref, const QStringList &mainNameSpace, const ObjectContext* ctx);
    static ObjectBase* resolveAnonymousReference(int ctxIndex, int refIndex);
    static ObjectBase* resolveAnonymousReference(const AnonymousObjectReference& ref) {
        return resolveAnonymousReference(ref.ctxIndex, ref.refIndex);
    }

private:
    static QHash<int, ObjectContext*> ContextRecord;
    static int ContextIndex;

private:
    void loadAllObjectsFromDirectory();

private:
    QString mainDirectory;
    QList<ObjectBase*> objects;
    QHash<QString, ObjectBase*> objectMap;
    QHash<ObjectBase*, int> objectToRefIndexMap;
    QHash<int, ObjectBase*> refIndexToObjectMap;
    const int ctxIndex;
    int nextReferenceIndex = 0;

public:
    decltype(objects.begin())   begin() {return objects.begin();}
    decltype(objects.end())     end()   {return objects.end();}

    decltype(objects.size())    size()  const {return objects.size();}
};

#endif // OBJECTCONTEXT_H
